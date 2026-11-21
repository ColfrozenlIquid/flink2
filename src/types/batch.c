#include "../../include/types/types.h"
#include "../../include/util.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t batch_layout(const schema_t* schema, size_t col_offset[16]) {
    size_t offset = 0;
    for (size_t i = 0; i < schema->count; i++) {
        col_offset[i] = offset;
        size_t bytes = (size_t)BATCH_SIZE * type_size(schema->fields[i].type);
        offset += (bytes + 63) & ~(size_t)63;
    }
    return offset;
}

void batch_init(schema_t* schema, uint8_t* base, batch_t* out, const size_t* col_offset) {
    out->schema = schema;
    out->rows_count = 0;
    out->capacity = BATCH_SIZE;

    for (size_t i = 0; i < schema->count; i++) {
        out->cols[i] = base + col_offset[i];
    }
    for (size_t i = schema->count; i < 16; i++) {
        out->cols[i] = NULL;
    }
}
