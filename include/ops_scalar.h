#pragma once
#include <stddef.h>
#include <stdint.h>

// Write the indices of rows where `price > thresh` into `sel_out` and return the count.
size_t filter_gt(const float* price, size_t n, float thresh, uint32_t* sel_out);

// Multiply each value from the input by the `factor`. Results are mapped into `out`.
void map_mul(const float* in, size_t n, float factor, float* out);

// Aggregate all input values and return the sum.
double agg_sum(const float* in, size_t n);

size_t filter_gt_branchless(const float* price, size_t n, float thresh, uint32_t* sel_out);

double agg_sum_4acc(const float* in, size_t n);
