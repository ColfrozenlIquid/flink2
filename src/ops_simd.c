#include "../include/ops_simd.h"
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>

double agg_sum_avx2(const float* in, size_t n) {
    __m256 s0 = _mm256_setzero_ps();
    __m256 s1 = _mm256_setzero_ps();
    __m256 s2 = _mm256_setzero_ps();
    __m256 s3 = _mm256_setzero_ps();

    size_t i = 0;

    for (; i + 32 <= n; i += 32) {
        s0 = _mm256_add_ps(s0, _mm256_loadu_ps(in + i));
        s1 = _mm256_add_ps(s1, _mm256_loadu_ps(in + i + 8));
        s2 = _mm256_add_ps(s2, _mm256_loadu_ps(in + i + 16));
        s3 = _mm256_add_ps(s3, _mm256_loadu_ps(in + i + 24));
    }

    __m256 s = _mm256_add_ps(_mm256_add_ps(s0, s1), _mm256_add_ps(s2, s3));

    float lanes[8];
    _mm256_storeu_ps(lanes, s);
    double total = 0;
    for (int k = 0; k < 8; k++) {
        total += lanes[k];
    }
    for (; i < n; i++) {
        total += in[i];
    }
    return total;
}

void filter_init(void) {
    for (int m = 0; m < 256; m++) {
        int k = 0;
        for (int b = 0; b < 8; b++) {
            if (m & (1 << b)) {
                lut[m][k++] = (uint32_t)b;
            }
        }
        for (; k < 8; k++){
            lut[m][k]= 0;
        }
    }
}

size_t filter_gt_avx2(const float* price, size_t n, float thresh, uint32_t* sel_out) {
    __m256 t = _mm256_set1_ps(thresh);
    __m256i idx = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
    const __m256i step = _mm256_set1_epi32(8);

    size_t found = 0, i = 0;

    for (; i + 8 <= n; i += 8) {
        __m256 v = _mm256_loadu_ps(price + i);
        int mask = _mm256_movemask_ps(_mm256_cmp_ps(v, t, _CMP_GT_OQ));
        __m256i recipe = _mm256_load_si256((const __m256i*)lut[mask]);
        __m256i packed = _mm256_permutevar8x32_epi32(idx, recipe);
        _mm256_storeu_si256((__m256i*)(sel_out + found), packed);
        found += (size_t)__builtin_popcount((unsigned)mask);
        idx = _mm256_add_epi32(idx, step);
    }

    for (; i < n; i++) {
        sel_out[found] = (uint32_t)i;
        found += (price[i] > thresh);
    }

    return found;
}
