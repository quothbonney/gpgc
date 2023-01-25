#include "gpgc.hpp"

int** gpgc_create_matrix_A(int partition_size) {
	int** matrix_a;
	matrix_a = (int**)malloc(partition_size * partition_size * sizeof(int*));
	for(int i = 0; i < partition_size; ++i) {
		for(int j = 0; j < partition_size; ++j) {
			int current_index = (i * partition_size) + j;
            matrix_a[current_index] = (int*)malloc(3 * sizeof(int));
			int row[3] = {i, j, 1};
			matrix_a[current_index] = row;
        }
	}
    return matrix_a;
}

int** gpgc_get_transpose(int** matrix_a, int rows, int cols) {
	int** transpose_matrix = (int**)malloc(cols * sizeof(int*));
	for(int i1 = 0; i1 < cols; i1++)
		transpose_matrix[i1] = (int*)malloc(rows * sizeof(int));
	for(int i = 0; i < rows; i++) {
		for(int j = 0; j < cols; j++) {
			transpose_matrix[j][i] = matrix_a[i][j];
		}
	}
}

int** gpgc_multiply_matricies(int** arr1, int** arr2, int rows, int cols) {
	int** product = (int**)malloc(rows * sizeof(int*));
	for(int i = 0; i < rows; i++) {
		product[i] = (int*)malloc(rows * sizeof(int));
		for(int j = 0; j < rows; j++) {
			product[i][j] = 0;
			for(int k = 0; k < cols; k++) {
				product[i][j] += arr1[i][k] * arr2[k][j];
			}
		}
	}
	return product;
}
