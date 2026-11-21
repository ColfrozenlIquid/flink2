#include "../include/ops_scalar.h"
#include "../include/ops_simd.h"
#include "../include/source/reader.h"
#include "../include/types/types.h"
#include "../include/util.h"
#include "math.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static inline double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

typedef struct {
    size_t batches, rows_in, rows_kept;
    double total;
    double t_read, t_filter, t_gather, t_map, t_sum; // seconds
} stats_t;

int run_pipeline(
    source_t src,
    const wire_layout_t* wire,
    const schema_t* schema,
    float threshold,
    float factor,
    int flush_ms,
    stats_t* st
) {
    int rc = -1;
    batch_pool_t pool;
    reader_t reader;
    float* tmp = aligned_alloc(64, BATCH_SIZE * sizeof(float));
    float* mapped = aligned_alloc(64, BATCH_SIZE * sizeof(float));
    uint32_t* selected = aligned_alloc(64, BATCH_SIZE * sizeof(uint32_t));
    if (!tmp || !mapped || !selected)
        goto out_scratch;

    int price_col = schema_find((schema_t*)schema, "price", 5);
    if (price_col < 0)
        goto out_scratch;

    if (pool_init(&pool, schema, 2) != 0)
        goto out_scratch;
    if (reader_init(&reader, src, wire, schema, 64 * 1024) != 0)
        goto out_pool;

    memset(st, 0, sizeof *st);

    for (;;) {
        batch_t* b = pool_acquire(&pool);
        if (!b)
            break;

        double t0 = now_sec();
        ssize_t n = reader_fill_batch(&reader, b, flush_ms);
        double t1 = now_sec();
        if (n <= 0) {
            pool_release(&pool, b);
            rc = (n < 0) ? -1 : 0;
            break;
        }

        const float* price = b->cols[price_col];
        size_t rows = b->rows_count;

        size_t kept = filter_gt_avx2(price, rows, threshold, selected);
        double t2 = now_sec();

        for (size_t i = 0; i < kept; i++)
            tmp[i] = price[selected[i]];
        double t3 = now_sec();

        map_mul(tmp, kept, factor, mapped);
        double t4 = now_sec();

        st->total += agg_sum_avx2(mapped, kept);
        double t5 = now_sec();

        st->t_read += t1 - t0;
        st->t_filter += t2 - t1;
        st->t_gather += t3 - t2;
        st->t_map += t4 - t3;
        st->t_sum += t5 - t4;

        st->batches++;
        st->rows_in += rows;
        st->rows_kept += kept;

        pool_release(&pool, b);
    }

    reader_destroy(&reader);
out_pool:
    pool_destroy(&pool);
out_scratch:
    free(tmp);
    free(mapped);
    free(selected);
    return rc;
}

#define PAYLOAD (1u << 22) // 4M rows, about 32 MB on the wire

#pragma pack(push, 1)
typedef struct {
    int32_t user_id;
    float price;
} wire_row_t; // 8 bytes per row
#pragma pack(pop)

int main(void) {
    filter_init();
    // 1. schema: what the engine stores in memory
    schema_t schema;
    schema.count = 0;
    schema_add_field(&schema, "user_id", 7, T_I32);
    schema_add_field(&schema, "price", 5, T_F32);

    // 2. wire layout: what the sender puts on the socket
    wire_layout_t wire = {0};
    wire.row_size = sizeof(wire_row_t);
    wire.offset[0] = offsetof(wire_row_t, user_id);
    wire.offset[1] = offsetof(wire_row_t, price);

    // 3. fake incoming stream (same generator as your old benchmark)
    wire_row_t* rows = malloc((size_t)PAYLOAD * sizeof *rows);
    if (!rows) {
        fprintf(stderr, "alloc failed\n");
        return 1;
    }

    uint64_t rng = 88172645463325252ULL;
    const float threshold = 100.0f, factor = 1.19f;
    size_t ref_kept = 0;
    double ref_total = 0;

    for (size_t i = 0; i < PAYLOAD; i++) {
        rows[i].user_id = (int32_t)((uint32_t)(xorshift64(&rng) >> 32) % 1000);
        rows[i].price = ((uint32_t)(xorshift64(&rng) >> 32) % 20000) / 100.0f;
        if (rows[i].price > threshold) { // scalar reference answer
            ref_kept++;
            ref_total += (double)(rows[i].price * factor);
        }
    }

    // 4. source: max_chunk = 1460 mimics TCP segments, so rows get split mid-field
    mem_source_t m = {
        .data = (const uint8_t*)rows,
        .len = (size_t)PAYLOAD * sizeof *rows,
        .pos = 0,
        .max_chunk = 1460,
    };

    enum { REPS = 10 };
    stats_t best = {0};
    double best_wall = 1e30;

    for (int rep = 0; rep < REPS; rep++) {
        m.pos = 0; // rewind the source each repetition
        stats_t st;
        double t0 = now_sec();
        int rc = run_pipeline((source_t){mem_read, &m}, &wire, &schema, threshold, factor, 50, &st);
        double wall = now_sec() - t0;
        if (rc != 0) {
            fprintf(stderr, "pipeline failed\n");
            free(rows);
            return 1;
        }
        if (wall < best_wall) {
            best_wall = wall;
            best = st;
        } // keep the best whole run
    }

    // baseline: one bulk memcpy of the same bytes
    size_t bytes = (size_t)PAYLOAD * sizeof *rows;
    uint8_t* dst = malloc(bytes);
    double best_copy = 1e30;
    for (int rep = 0; rep < REPS; rep++) {
        double t0 = now_sec();
        memcpy(dst, rows, bytes);
        double dt = now_sec() - t0;
        if (dt < best_copy)
            best_copy = dt;
        __asm__ volatile("" : : "r"(dst) : "memory"); // keep the compiler from removing it
    }

    double stages = best.t_read + best.t_filter + best.t_gather + best.t_map + best.t_sum;
    printf(
        "batches=%zu rows_in=%zu rows_kept=%zu total=%.2f\n", best.batches, best.rows_in,
        best.rows_kept, best.total
    );
    printf("reference:     rows_kept=%zu total=%.2f\n", ref_kept, ref_total);
    printf("wall: %.3f ms, %.1f M rows/s\n", best_wall * 1e3, best.rows_in / best_wall / 1e6);
    printf("\nstage      ms      %%\n");
    printf("read+dec  %7.3f  %5.1f\n", best.t_read * 1e3, 100 * best.t_read / stages);
    printf("filter    %7.3f  %5.1f\n", best.t_filter * 1e3, 100 * best.t_filter / stages);
    printf("gather    %7.3f  %5.1f\n", best.t_gather * 1e3, 100 * best.t_gather / stages);
    printf("map       %7.3f  %5.1f\n", best.t_map * 1e3, 100 * best.t_map / stages);
    printf("sum       %7.3f  %5.1f\n", best.t_sum * 1e3, 100 * best.t_sum / stages);
    printf(
        "\nbulk memcpy of %.1f MB: %.3f ms (%.1f GB/s)\n", bytes / 1e6, best_copy * 1e3,
        bytes / best_copy / 1e9
    );

    size_t chunks[] = {1460, 16384, 65536, 0};
    for (size_t k = 0; k < sizeof chunks / sizeof *chunks; k++) {
        m.max_chunk = chunks[k];
        double best = 1e30;
        stats_t bst = {0};
        for (int rep = 0; rep < REPS; rep++) {
            m.pos = 0;
            stats_t st;
            run_pipeline((source_t){mem_read, &m}, &wire, &schema, threshold, factor, 50, &st);
            if (st.t_read < best) {
                best = st.t_read;
                bst = st;
            }
        }
        printf("max_chunk=%6zu  read+dec=%.3f ms\n", chunks[k], best * 1e3);
    }

    int ok = best.rows_in == PAYLOAD && best.rows_kept == ref_kept &&
             fabs(best.total - ref_total) <= 1e-3 * fabs(ref_total);
    printf("%s\n", ok ? "OK" : "MISMATCH");

    free(dst);
    free(rows);
    return ok ? 0 : 1;
}
