#include "../../include/source/reader.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int reader_init(
    reader_t* reader,
    source_t source,
    const wire_layout_t* wire,
    const schema_t* schema,
    size_t buf_cap
) {
    memset(reader, 0, sizeof(reader_t));
    if (wire->row_size == 0 || buf_cap < wire->row_size) {
        return -1;
    }

    for (size_t i = 0; i < schema->count; i++) {
        if (wire->offset[i] + type_size(schema->fields[i].type) > wire->row_size)
            return -1;
    }

    reader->buf = malloc(buf_cap);
    if (!reader->buf)
        return -1;

    reader->src = source;
    reader->wire = *wire;
    reader->cap = buf_cap;
    return 0;
}

void reader_destroy(reader_t* r) {
    free(r->buf);
    r->buf = NULL;
}

ssize_t reader_fill_batch(reader_t* reader, batch_t* batch, int flush_ms) {
    const size_t row_size = reader->wire.row_size;
    size_t n = 0;

    for (;;) {
        // How many rows are buffered in the reader
        size_t available = (reader->end - reader->start) / row_size;
        if (available > batch->capacity - n) {
            available = batch->capacity - n;
        }
        if (available) {
            decode_rows(reader, batch, n, available);
            reader->start += available * row_size;
            n += available;
        }
        if (n == batch->capacity) {
            break;
        }

        size_t left = reader->end - reader->start;
        memmove(reader->buf, reader->buf + reader->start, left);
        reader->start = 0;
        reader->end = left;

        ssize_t got = reader->src.read(
            reader->src.ctx, reader->buf + reader->end, reader->cap - reader->end, n ? flush_ms : -1
        );

        if (got == SRC_EOF) {
            break;
        }
        if (got == SRC_ERR) {
            return -1;
        }
        if (got == SRC_AGAIN) {
            if (n) {
                break;
            }
            continue;
        }
        reader->end += (size_t)got;
    }
    batch->rows_count = n;
    return (ssize_t)n;
}

void decode_rows(const reader_t* reader, batch_t* batch, size_t dst, size_t count) {
    const uint8_t* src = reader->buf + reader->start;
    const schema_t* s = batch->schema;

    for (size_t col = 0; col < s->count; col++) {
        const uint8_t* in = src + reader->wire.offset[col];
        size_t row_size = reader->wire.row_size;

        switch (s->fields[col].type) {
        case T_I32: {
            int32_t* out = (int32_t*)batch->cols[col] + dst;
            for (size_t i = 0; i < count; i++) {
                memcpy(&out[i], in + i * row_size, 4);
            }
        } break;
        case T_I64: {
            int64_t* out = (int64_t*)batch->cols[col] + dst;
            for (size_t i = 0; i < count; i++) {
                memcpy(&out[i], in + i * row_size, 8);
            }
        } break;
        case T_F32: {
            float* out = (float*)batch->cols[col] + dst;
            for (size_t i = 0; i < count; i++) {
                memcpy(&out[i], in + i * row_size, 4);
            }
        } break;
        case T_F64: {
            double* out = (double*)batch->cols[col] + dst;
            for (size_t i = 0; i < count; i++) {
                memcpy(&out[i], in + i * row_size, 8);
            }
        } break;
        default:
            break;
        }
    }
}
