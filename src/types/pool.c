#include "../../include/types/types.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int pool_init(batch_pool_t* pool, const schema_t* schema, size_t pool_size) {
    pool->schema = *schema;
    pool->pool_size = pool_size;
    pool->batch_bytes = batch_layout(schema, pool->col_offset);

    // This gives us a continuous slab of 64 Byte aligned memory batches, sized to the pool size
    pool->slab = aligned_alloc(64, pool->batch_bytes * pool_size);
    pool->batches = malloc(pool_size * sizeof(batch_t));
    pool->free_list = malloc(pool_size * sizeof(batch_t*));

    if (!pool->slab || !pool->batches || !pool->free_list) {
        free(pool->slab);
        free(pool->free_list);
        free(pool->batches);
        return -1;
    }

    for (size_t i = 0; i < pool_size; i++) {
        batch_init(
            &pool->schema, pool->slab + i * pool->batch_bytes, &pool->batches[i], pool->col_offset
        );
        pool->free_list[i] = &pool->batches[i];
    }

    pool->free_count = pool_size;

    return 0;
}

void pool_destroy(batch_pool_t* pool) {
    free(pool->slab);
    free(pool->batches);
    free(pool->free_list);
}

batch_t* pool_acquire(batch_pool_t* pool) {
    // Check the free count. If the free count > 0, return a pointer to a batch on the stack and
    // decrement the free count. This pops the batch off the stack of available batches. Otherwise
    // return NULL if no batch is available right now.
    return pool->free_count ? pool->free_list[--pool->free_count] : NULL;
}

void pool_release(batch_pool_t* pool, batch_t* batch) {
    batch->rows_count = 0;
    // Push the batch onto the stack of free batches ready to be acquired
    pool->free_list[pool->free_count++] = batch;
}
