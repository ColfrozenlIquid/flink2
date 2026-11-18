#include "../include/ops_scalar.h"
#include <stddef.h>
#include <stdint.h>

size_t filter_gt(const float* price, size_t n, float thresh, uint32_t* sel_out) {
    size_t found = 0;
    for (size_t i = 0; i < n; i++) {
        if (price[i] > thresh) {
            sel_out[found] = i;
            found++;
        }
    }
    return found;
}

size_t filter_gt_branchless(const float* price, size_t n, float thresh, uint32_t* sel_out) {
    size_t found = 0;
    for (size_t i = 0; i < n; i++) {
        sel_out[found] = (uint32_t)i;
        found += (price[i] > thresh);
    }
    return found;
}

void map_mul(const float* restrict in, size_t n, float factor, float* restrict out) {
    for (size_t i = 0; i < n; i++) {
        out[i] = in[i] * factor;
    }
}

double agg_sum(const float* in, size_t n) {
    double agg = 0;
    for (size_t i = 0; i < n; i++) {
        agg += in[i];
    }
    return agg;
}

double agg_sum_4acc(const float* in, size_t n) {
    double a0, a1, a2, a3;
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        a0 += in[i];
        a1 += in[i + 1];
        a2 += in[i + 2];
        a3 += in[i + 3];
    }
    for (; i < n; i++) {
        a0 += in[i];
    }
    return a0 + a1 + a2 + a3;
}
