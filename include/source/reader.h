#pragma once
#include "../types/types.h"
#include "source.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    size_t row_size;                   // bytes per row on the wire
    size_t offset[SCHEMA_FIELD_COUNT]; // byte offset for each field within a row
} wire_layout_t;

typedef struct {
    source_t src;
    uint8_t* buf;
    size_t cap;
    size_t start;
    size_t end;
    wire_layout_t wire;
} reader_t;

ssize_t reader_fill_batch(reader_t* reader, batch_t* batch, int flush_ms);

void decode_rows(const reader_t* reader, batch_t* batch, size_t dst, size_t count);

int reader_init(
    reader_t* reader,
    source_t source,
    const wire_layout_t* wire,
    const schema_t* schema,
    size_t buf_cap
);

void reader_destroy(reader_t* r);
