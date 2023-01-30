#include "gpgc.hpp"

int** gpgc_create_matrix_A(int partition_size, int skipper) {
	int** matrix_a;
	int m_sz = partition_size / skipper;
	matrix_a = (int**)malloc(m_sz * m_sz * sizeof(int*));

	for(int i = 0; i < m_sz; i++) {
		for(int j = 0; j < m_sz; j++) {
			int current_index = (i * m_sz) + j;
            matrix_a[current_index] = (int*)malloc(3 * sizeof(int));
            int row_arr[] = {i * skipper, j * skipper, 1};
            for(int l = 0; l < 3; l++) {
                matrix_a[current_index][l] = row_arr[l];
            }
        }
	}
    return matrix_a;
}

int** gpgc_create_matrix_B(int partition_size, int skipper, int* block) {
	int** matrix_b;
	int m_sz = partition_size / skipper;
	matrix_b = (int**)malloc(m_sz * m_sz * sizeof(int*));

	for(int i = 0; i < m_sz*m_sz; i++) {
		matrix_b[i] = (int*)malloc(sizeof(int));
		matrix_b[i] = &block[i*skipper];
	}
    return matrix_b;
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
    return transpose_matrix;
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
