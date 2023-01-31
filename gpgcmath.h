//
// Created by quothbonney on 1/30/23.
//

#ifndef GPGC_GPGCMATH_H
#define GPGC_GPGCMATH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int* gpgc_create_matrix_B(int partition_size, int skipper, const float* block);

int* gpgc_create_matrix_A(int partition_size, int skipper, const float* block);

#endif //GPGC_GPGCMATH_H
#ifdef __cplusplus
} // extern "C"
#endif