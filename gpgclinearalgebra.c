#include "gpgcmath.h"


int* gpgc_create_eigen_A(int partition_size, int skipper, const float* block) {
    int* arr_A;
    int m_sz = partition_size / skipper;
    arr_A = (int*)malloc((3 * m_sz * m_sz) * sizeof(int));
    int p = 0;
    int x_count = 0;
    while (p < (m_sz * m_sz * 3) - 3) {
        for(int i = 0; i < m_sz; ++i) {
            arr_A[p++] = x_count * skipper;
            arr_A[p++] = i * skipper;
            arr_A[p++] = 1;
        }
        x_count++;
    }
    return arr_A;
}

int* gpgc_create_matrix_B(int partition_size, int skipper, const float* block) {
	int* matrix_b;
	int m_sz = partition_size / skipper;
	matrix_b = (int*)malloc(m_sz * m_sz * sizeof(int*));

	for(int i = 0; i < m_sz*m_sz; i++) {
		matrix_b[i] = (int)block[i*skipper];
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

int** gpgc_restructure_block(const float* block_arr, int sq_size, int skipper) {
    int** matrix;
    int m_sz = sq_size / skipper;
    matrix = (int**)malloc(m_sz * sizeof(int*));

    for(int i = 0; i < m_sz; i++) {
        matrix[i] = (int*)malloc(m_sz * sizeof(int));
        for(int j = 0; j < m_sz; j++) {
            int current_index = (i * m_sz) + j;
            matrix[i][j] = block_arr[current_index];
        }
    }
    return matrix;
}

void least_squares_regression(int** data, int rows, int cols, double* a, double* b, double* c) {
    unsigned long long x_sum = 0, y_sum = 0, z_sum = 0;
    unsigned long long x_squared_sum = 0, y_squared_sum = 0;
    unsigned long long x_y_sum = 0, x_z_sum = 0, y_z_sum = 0;

    for (int i = 0; i < rows; i++) {
        x_sum += data[i][0];
        y_sum += data[i][1];
        z_sum += data[i][2];

        x_squared_sum += pow(data[i][0], 2);
        y_squared_sum += pow(data[i][1], 2);

        x_y_sum += data[i][0] * data[i][1];
        x_z_sum += data[i][0] * data[i][2];
        y_z_sum += data[i][1] * data[i][2];
    }

    double denominator = (rows * x_squared_sum) - pow(x_sum, 2);
    *a = ((y_z_sum * x_squared_sum) - (x_y_sum * x_z_sum)) / denominator;
    *b = ((z_sum * x_squared_sum) - (x_sum * x_z_sum)) / denominator;
    *c = ((rows * x_y_sum) - (x_sum * y_sum)) / denominator;
}

void least_squares_regression_filled(int rows, int cols, int** A, int* B, int* X) {
    int ATA[cols][cols];
    int ATB[cols];

    // Initialize ATA and ATB
    for (int i = 0; i < cols; i++) {
        ATB[i] = 0;
        for (int j = 0; j < cols; j++) {
            ATA[i][j] = 0;
            for (int k = 0; k < rows; k++) {
                ATA[i][j] += A[k][i] * A[k][j];
            }
        }
        for (int k = 0; k < rows; k++) {
            ATB[i] += A[k][i] * B[k];
        }
    }

    // Solve for X using Gaussian elimination
    for (int i = 0; i < cols; i++) {
        int max_index = i;
        int max_val = ATA[i][i];

        // Find the maximum value in the current column
        for (int j = i + 1; j < cols; j++) {
            if (fabs(ATA[j][i]) > fabs(max_val)) {
                max_index = j;
                max_val = ATA[j][i];
            }
        }

        // Swap the current row with the row containing the maximum value
        if (i != max_index) {
            for (int j = 0; j < cols; j++) {
                int temp = ATA[i][j];
                ATA[i][j] = ATA[max_index][j];
                ATA[max_index][j] = temp;
            }
            int temp = ATB[i];
            ATB[i] = ATB[max_index];
            ATB[max_index] = temp;
        }

        // Eliminate the values below the current row
        for (int j = i + 1; j < cols; j++) {

            int factor;
            ATA[i][i] == 0 ? factor = 0 : factor = ATA[j][i] / ATA[i][i];
            for (int k = i; k < cols; k++) {
                ATA[j][k] -= factor * ATA[i][k];
            }
            ATB[j] -= factor * ATB[i];
        }
    }

    // Back-substitute to find X
    for (int i = cols - 1; i >= 0; i--) {
        X[i] = ATB[i];
        for (int j = i + 1; j < cols; j++) {
            X[i] -= ATA[i][j] * X[j];
        }
        ATA[i][i] == 0 ? X[i] = 0 : X[i] /= ATA[i][i];

    }
}
