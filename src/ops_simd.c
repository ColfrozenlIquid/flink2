#include "../include/ops_simd.h"
#include <immintrin.h>
#include <stddef.h>

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
