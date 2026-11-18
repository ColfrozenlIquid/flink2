#include "../include/ops_simd.h"
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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
        for (; k < 8; k++) {
            lut[m][k] = 0;
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

__attribute__((target("avx512f"))) size_t
filter_gt_avx512(const float* price, size_t n, float thresh, uint32_t* sel_out) {
    __m512 t = _mm512_set1_ps(thresh);
    __m512i idx = _mm512_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    const __m512i step = _mm512_set1_epi32(16);

    size_t found = 0, i = 0;
    for (; i + 16 <= n; i += 16) {
        __m512 v = _mm512_loadu_ps(price + i);
        __mmask16 m = _mm512_cmp_ps_mask(v, t, _CMP_GT_OQ);
        __m512i packed = _mm512_maskz_compress_epi32(m, idx);
        _mm512_storeu_si512(sel_out + found, packed);
        found += (size_t)__builtin_popcount((unsigned)m);
        idx = _mm512_add_epi32(idx, step);
    }
    for (; i < n; i++) {
        sel_out[found] = (uint32_t)i;
        found += (price[i] > thresh);
    }
    return found;
}

const char*
substr_avx2(const char* haystack, size_t hay_len, const char* needle, size_t needle_len) {
    if (needle_len == 0) {
        return haystack;
    }
    if (needle_len > hay_len) {
        return NULL;
    }
    if (needle_len == 1) {
        return memchr(haystack, needle[0], hay_len);
    }

    __m256i first = _mm256_set1_epi8(needle[0]);
    __m256i last = _mm256_set1_epi8(needle[needle_len - 1]);

    size_t i = 0;
    for (; i + needle_len - 1 + 32 <= hay_len; i += 32) {
        __m256i a = _mm256_loadu_si256((const __m256i*)(haystack + i));
        __m256i b = _mm256_loadu_si256((const __m256i*)(haystack + i + needle_len - 1));

        __m256i eq = _mm256_and_si256(_mm256_cmpeq_epi8(a, first), _mm256_cmpeq_epi8(b, last));

        unsigned mask = _mm256_movemask_epi8(eq);
        while (mask) {
            unsigned bit = __builtin_ctz(mask);
            if (needle_len >= 2 &&
                memcmp(haystack + i + bit + 1, needle + 1, needle_len - 2) == 0) {
                return haystack + i + bit;
            };
            mask &= mask - 1;
        }
    }
    for (; i + needle_len <= hay_len; i++)
        if (memcmp(haystack + i, needle, needle_len) == 0)
            return haystack + i;
    return NULL;
}
