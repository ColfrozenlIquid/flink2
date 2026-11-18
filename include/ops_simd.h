#pragma once
#include <stddef.h>
#include <stdint.h>

static uint32_t lut[256][8] __attribute__((aligned(32)));

void filter_init(void);

double agg_sum_avx2(const float* in, size_t n);

size_t filter_gt_avx2(const float* price, size_t n, float thresh, uint32_t* sel_out);
