#include "gpgcmath.h"

inline double inverse_z_transform(double z_score) {
	double pi = 3.1415;
    return (1 / sqrt(2*pi)) * exp(-(z_score*z_score)/2);
}

int* gpgc_create_matrix_A(int partition_size, int skipper, const int* block) {
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

int* gpgc_create_matrix_B(int partition_size, int skipper, const int* block) {
	int* matrix_b;
	int m_sz = partition_size / skipper;
	matrix_b = (int*)malloc(m_sz * m_sz * sizeof(int*));

	for(int i = 0; i < m_sz*m_sz; i++) {
		matrix_b[i] = (int)block[i*skipper];
	}
    return matrix_b;
}
