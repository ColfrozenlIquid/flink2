#include "../include/batch.h"
#include "../include/ops_scalar.h"
#include "../include/ops_simd.h"
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

int main() {
    Batch batch;
    batch_allocate(&batch, 32);

    uint32_t* all_user = aligned_alloc(32, PAYLOAD * sizeof(uint32_t));
    float* all_prices = aligned_alloc(32, PAYLOAD * sizeof(float));
    if (!all_user || !all_prices) {
        fprintf(stderr, "Allocation failed");
        return 1;
    }

    filter_init();



    uint64_t rng = 88172645463325252ULL;

    for (size_t i = 0; i < PAYLOAD; i++) {
        all_user[i] = (uint32_t)(xorshift64(&rng) >> 32) % 1000;
        all_prices[i] = ((uint32_t)(xorshift64(&rng) >> 32) % 20000) / 100.0f;
    }

    size_t kept = 0;
    double total = 0;

    float* mapped = aligned_alloc(32, BATCH_SIZE * (size_t)sizeof(float));
    uint32_t* selected = aligned_alloc(32, BATCH_SIZE * (size_t)sizeof(uint32_t));

    uint32_t ref[BATCH_SIZE];
    size_t n_ref = filter_gt_branchless(all_prices, BATCH_SIZE, 100.0f, ref);
    size_t n_simd = filter_gt_avx2(all_prices, BATCH_SIZE, 100.0f, selected);
    if (n_ref != n_simd || memcmp(ref, selected, n_ref * sizeof(uint32_t)) != 0) {
        fprintf(stderr, "filter mismatch!\n");
        return 1;
    }

    double best_filter = 1e30, best_map = 1e30, best_sum = 1e30;
    double best_total = 1e30;

    for (int i = 0; i < REPS; i++) {
        kept = 0;
        total = 0;
        double t_filter = 0, t_map = 0, t_sum = 0;

        for (size_t b = 0; b < PAYLOAD / BATCH_SIZE; b++) {
            const float* price = all_prices + b * BATCH_SIZE;

            double a = now_sec();
            kept += filter_gt_avx2(price, BATCH_SIZE, 100.0f, selected);
            double c = now_sec();
            map_mul(price, BATCH_SIZE, 1.19f, mapped);
            double d = now_sec();
            total += agg_sum_avx2(mapped, BATCH_SIZE);
            double e = now_sec();

            t_filter += c - a;
            t_map += d - c;
            t_sum += e - d;
        }

        if (t_filter < best_filter)
            best_filter = t_filter;
        if (t_map < best_map)
            best_map = t_map;
        if (t_sum < best_sum)
            best_sum = t_sum;

        double rep_total = t_filter + t_map + t_sum;
        if (rep_total < best_total)
            best_total = rep_total;
    }

    printf("kept=%zu total=%f\n", kept, total);
    printf("filter: %.3f ms\n", best_filter * 1e3);
    printf("map:    %.3f ms\n", best_map * 1e3);
    printf("sum:    %.3f ms\n", best_sum * 1e3);
    printf(
        "sum of operators: %.3f ms, %.1f M rows/s\n", best_total * 1e3, PAYLOAD / best_total / 1e6
    );

    batch_free(&batch);
    free(mapped);
    free(selected);

    return 0;
}
