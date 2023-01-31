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

int* gpgc_create_matrix_B(int partition_size, int skipper, const int* block);

int* gpgc_create_matrix_A(int partition_size, int skipper, const int* block);

double inverse_z_transform(double z_score);

#endif //GPGC_GPGCMATH_H
#ifdef __cplusplus
} // extern "C"
#endif
