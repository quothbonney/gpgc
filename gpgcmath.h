/*
 * Copyright (c) 2023, Jack David Carson - https://github.com/quothbonney
 * SPDX-License-Identifier: MIT
 */

#ifndef GPGC_GPGCMATH_H
#define GPGC_GPGCMATH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Calculates matrix A (explained in paper) to be passed by pointer to Eigen::Dense initializer
int* gpgc_create_matrix_A(int partition_size, int skipper, const int* block);

// Calculates matrix B for Eigen::Dense initializer
int* gpgc_create_matrix_B(int partition_size, int skipper, const int* block);

// Calculates integral of probability density function to convert z score to actual probability
double inverse_z_transform(double z_score);

#endif //GPGC_GPGCMATH_H
#ifdef __cplusplus
} // extern "C"
#endif
