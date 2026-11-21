#include "../include/ops_scalar.h"
#include "../include/ops_simd.h"
#include "../include/source/reader.h"
#include "../include/types/types.h"
#include "../include/util.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PAYLOAD (4096 * 16)
#define REPS 500

static inline double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

#pragma pack(push, 1)
typedef struct {
    int32_t user_id;
    float price;
} wire_row_t; // 8 bytes on the wire
#pragma pack(pop)

void create_pool(void) {
    batch_pool_t pool;
    reader_t reader;
    schema_t schema;
    schema.count = 0;
    schema_add_field(&schema, "user_id", 7, T_I32);
    schema_add_field(&schema, "price", 5, T_F32);
    if (pool_init(&pool, &schema, 5) != 0)
        return;

    // fake incoming stream
    wire_row_t rows[6];
    for (int i = 0; i < 6; i++)
        rows[i] = (wire_row_t){.user_id = 100 + i, .price = 1.5f * (float)(i + 1)};

    // max_chunk = 5 splits rows mid-field, like TCP could
    mem_source_t m = {.data = (const uint8_t*)rows, .len = sizeof rows, .max_chunk = 5};

    wire_layout_t wire = {0};
    wire.row_size = sizeof(wire_row_t);
    wire.offset[0] = offsetof(wire_row_t, user_id);
    wire.offset[1] = offsetof(wire_row_t, price);

    if (reader_init(&reader, (source_t){mem_read, &m}, &wire, &schema, 64) != 0) {
        pool_destroy(&pool);
        return;
    }

    batch_t* batch1 = pool_acquire(&pool);
    batch_t* batch2 = pool_acquire(&pool);
    printf("Free count after acquire: %zu\n", pool.free_count);

    ssize_t got = reader_fill_batch(&reader, batch1, -1);
    printf("filled %zd rows\n", got);
    // batch1->cols[0] is the int32 user_id column, cols[1] the float price column

    pool_release(&pool, batch1);
    pool_release(&pool, batch2);

    printf("Free count after release: %zu\n", pool.free_count);

    reader_destroy(&reader);
    pool_destroy(&pool);
}
int main() {
    create_pool();
    //     batch_t batch;
    //     batch_allocate(&batch, 32);
    //
    //     uint32_t* all_user = aligned_alloc(32, PAYLOAD * sizeof(uint32_t));
    //     float* all_prices = aligned_alloc(32, PAYLOAD * sizeof(float));
    //     if (!all_user || !all_prices) {
    //         fprintf(stderr, "Allocation failed");
    //         return 1;
    //     }
    //
    //     filter_init();
    //
    //     uint64_t rng = 88172645463325252ULL;
    //
    //     for (size_t i = 0; i < PAYLOAD; i++) {
    //         all_user[i] = (uint32_t)(xorshift64(&rng) >> 32) % 1000;
    //         all_prices[i] = ((uint32_t)(xorshift64(&rng) >> 32) % 20000) / 100.0f;
    //     }
    //
    //     size_t kept = 0;
    //     double total = 0;
    //
    //     float* mapped = aligned_alloc(32, BATCH_SIZE * (size_t)sizeof(float));
    //     uint32_t* selected = aligned_alloc(32, BATCH_SIZE * (size_t)sizeof(uint32_t));
    //
    //     uint32_t ref[BATCH_SIZE];
    //     size_t n_ref = filter_gt_branchless(all_prices, BATCH_SIZE, 100.0f, ref);
    //     size_t n_simd = filter_gt_avx2(all_prices, BATCH_SIZE, 100.0f, selected);
    //     if (n_ref != n_simd || memcmp(ref, selected, n_ref * sizeof(uint32_t)) != 0) {
    //         fprintf(stderr, "filter mismatch!\n");
    //         return 1;
    //     }
    //
    //     double best_filter = 1e30, best_map = 1e30, best_sum = 1e30;
    //     double best_total = 1e30;
    //
    //     for (int i = 0; i < REPS; i++) {
    //         kept = 0;
    //         total = 0;
    //         double t_filter = 0, t_map = 0, t_sum = 0;
    //
    //         for (size_t b = 0; b < PAYLOAD / BATCH_SIZE; b++) {
    //             const float* price = all_prices + b * BATCH_SIZE;
    //
    //             double a = now_sec();
    //             kept += filter_gt_avx2(price, BATCH_SIZE, 100.0f, selected);
    //             double c = now_sec();
    //             map_mul(price, BATCH_SIZE, 1.19f, mapped);
    //             double d = now_sec();
    //             total += agg_sum_avx2(mapped, BATCH_SIZE);
    //             double e = now_sec();
    //
    //             t_filter += c - a;
    //             t_map += d - c;
    //             t_sum += e - d;
    //         }
    //
    //         if (t_filter < best_filter)
    //             best_filter = t_filter;
    //         if (t_map < best_map)
    //             best_map = t_map;
    //         if (t_sum < best_sum)
    //             best_sum = t_sum;
    //
    //         double rep_total = t_filter + t_map + t_sum;
    //         if (rep_total < best_total)
    //             best_total = rep_total;
    //     }
    //
    //     printf("kept=%zu total=%f\n", kept, total);
    //     printf("filter: %.6f ms\n", best_filter * 1e3);
    //     printf("map:    %.6f ms\n", best_map * 1e3);
    //     printf("sum:    %.6f ms\n", best_sum * 1e3);
    //     printf(
    //         "sum of operators: %.6f ms, %.1f M rows/s\n", best_total * 1e3, PAYLOAD / best_total
    //         / 1e6
    //     );
    //
    //     batch_free(&batch);
    //     free(mapped);
    //     free(selected);

    return 0;
}
