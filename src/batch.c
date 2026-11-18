#include "../include/batch.h"
#include "../include/util.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int batch_allocate(Batch* out, size_t n) {
    if (n == 0 || n > BATCH_SIZE) {
        fprintf(stderr, "Illegal batch size: %zu (1..%d)\n", n, BATCH_SIZE);
        return -1;
    }

    out->user_id = allocate_column(n, sizeof(*out->user_id));
    out->price = allocate_column(n, sizeof(*out->price));
    if (!out->user_id || !out->price) {
        fprintf(stderr, "Failed to allocate memory\n");
        free(out->price);
        free(out->user_id);
        out->price = NULL;
        out->user_id = NULL;
        return -1;
    }

    out->len = 0;
    out->capacity = n;

    return 0;
}

void* allocate_column(size_t rows, size_t element_size) {
    if (element_size && rows > (SIZE_MAX - 31) / element_size) {
        return NULL;
    }

    // Convert n to a multiple of 32
    size_t bytes = (rows * element_size + 31) & ~(size_t)31;
    void* p = aligned_alloc(32, bytes);
    if (p) {
        memset(p, 0, bytes);
    }
    return p;
}

void batch_free(Batch* in) {
    free(in->price);
    free(in->user_id);
    in->user_id = NULL;
    in->price = NULL;
    in->len = 0;
    in->capacity = 0;
}

void batch_fill_random(Batch* in, size_t n, uint64_t* rng_state) {
    for (size_t i = 0; i < n; i++) {
        in->user_id[i] =
            (uint32_t)(xorshift64(rng_state) >> 32) % 1000; // user_id between 0 and 1000
        in->price[i] = ((uint32_t)(xorshift64(rng_state) >> 32) % 20000) /
                       100.0f; // price between 0.00 and 199.99
    }
    in->len = n;
}
