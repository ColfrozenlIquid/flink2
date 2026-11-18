#pragma once
#include <stddef.h>
#include <stdint.h>

#define BATCH_SIZE 4096

typedef struct {
    uint32_t* user_id;
    float* price;
    size_t len;      // rows currently valid
    size_t capacity; // rows allocated
} Batch;

int batch_allocate(Batch* out, size_t n);

void* allocate_column(size_t rows, size_t element_size);

void batch_free(Batch* in);

void batch_fill_random(Batch* in, size_t n, uint64_t* rng_state);
