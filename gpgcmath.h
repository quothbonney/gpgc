//
// Created by quothbonney on 1/30/23.
//

#ifndef GPGC_GPGCMATH_H
#define GPGC_GPGCMATH_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int** gpgc_create_matrix_A(int partition_size, int skipper, const float* block);
int** gpgc_get_transpose(int** matrix_a, int rows, int cols);
int** gpgc_restructure_block(const float* block_arr, int sq_size, int skipper);
void least_squares_regression(int** data, int rows, int cols, double* a, double* b, double* c);
int** gpgc_multiply_matrices(int** arr1, int** arr2, int rows1, int cols1, int cols2);
int* gpgc_create_matrix_B(int partition_size, int skipper, const float* block);
void least_squares_regression_filled(int rows, int cols, int** A, int* B, int* X);
int* gpgc_create_eigen_A(int partition_size, int skipper, const float* block);
#endif //GPGC_GPGCMATH_H