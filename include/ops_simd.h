#pragma once
#include <stddef.h>
#include <stdint.h>

static uint32_t lut[256][8] __attribute__((aligned(32)));

void filter_init(void);

double agg_sum_avx2(const float* in, size_t n);

size_t filter_gt_avx2(const float* price, size_t n, float thresh, uint32_t* sel_out);

size_t filter_gt_avx512(const float* price, size_t n, float thresh, uint32_t* sel_out);

const char*
substr_avx2(const char* haystack, size_t hey_len, const char* needle, size_t needle_len);
