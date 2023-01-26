#ifndef GPGC_H
#define GPGC_H

#include <cstdint>
#include <ostream>
#include <stdio.h>
#include <iostream>
#include <string>
#include <eigen3/Eigen/Dense>
#include <fstream>
#include <filesystem>
#include <sys/types.h>
#include "half/half/half.hpp"
#include "gdal_priv.h"

#define GPGC_HEADER_SIZE 8 

namespace gpgc_compression_paramters {
	static int gpgc_max_error;
	static int gpgc_mu;
	static float gpgc_zeta;
}

struct gpgc_header_t {
	uint32_t width;
	uint32_t height;
};

struct gpgc_vector{
	half_float::half i, j;
    int16_t k;
    u_int16_t size;
};

struct raster_offset {
    int x, y, size;

    raster_offset(int _x,int _y,int _s) : x(_x), y(_y), size(_s) {};
};

typedef struct {
	uint16_t height, width;
	GDALRasterBand* rBand;
	double rHeaderData[6];
} gpgc_gdal_data;

typedef struct {
	unsigned int width;
	unsigned int height;
} gpgc_desc;

typedef struct {
   unsigned char* bytestream;
   int p;
   std::ofstream ez_enc;
} gpgc_encoder;

void gpgc_encode_64(gpgc_encoder *_gpe, const uint64_t &serialized);
void gpgc_easy_write(gpgc_encoder *_gpe, gpgc_vector fit, int size);
double inverse_z_transform(double z_score);
int** gpgc_create_matrix_A(int partition_size, int skipper);
int** gpgc_get_transpose(int** matrix_a, int rows, int cols);
int** gpgc_multiply_matricies(int** arr1, int** arr2, int rows, int cols);

struct gpgc_partition {
	int xOff, yOff, size;
	uint16_t** bmp;
	gpgc_partition(int _size, int _xoff, int _yoff, uint16_t** rasterBMP, gpgc_encoder* encoder_data);

private:
	float get_entropy(const gpgc_vector& vec, const float* block) const;

	void subpartition(float entropy, gpgc_encoder* _gpe, const gpgc_vector* _encoded_vector);

	float* get_block() const;

	gpgc_vector fit_vector(const float* block);

};

inline void gpgc_encode_64(gpgc_encoder* _gpe, const uint64_t& serialized) {
	_gpe->bytestream[(_gpe->p)++] = (0xFFFF000000000000 & serialized) >> 48;
	_gpe->bytestream[(_gpe->p)++] = (0x0000FFFF00000000 & serialized) >> 32;
	_gpe->bytestream[(_gpe->p)++] = (0x00000000FFFF0000 & serialized) >> 16;
	_gpe->bytestream[(_gpe->p)++] = (0x000000000000FFFF & serialized);
}

inline void gpgc_encode_size(gpgc_encoder* _gpe, const uint8_t iteration_size) {
    _gpe->bytestream[(_gpe->p)++] = (0xFFFF & iteration_size);
    _gpe->bytestream[(_gpe->p)++] = (0x737A) >> 8; // Hints at next character being size
}

inline void gpgc_easy_write(gpgc_encoder* _gpe, gpgc_vector fit, int size) {
    _gpe->ez_enc << fit.i << " " << fit.j << " " << fit.k << " " << size << "\n";
}

inline void gpgc_easy_size(gpgc_encoder* _gpe, int size, int num) {
    _gpe ->ez_enc << "SIZEIS: " << size << " " << num << "\n";
}

inline void gpgc_decode_64(gpgc_encoder* _gpe) {
	unsigned int a = _gpe->bytestream[(_gpe->p)++];
}

gpgc_gdal_data process_file(const char* filename);

inline double inverse_z_transform(double z_score);

uint16_t** gpgc_read_16(const gpgc_gdal_data* rData);

inline int maxl2(int size);

std::vector<raster_offset> iteration_map(int raster_size, int it_size, int offX, int offY);

void gpgc_encode(char* filename, char* out_filename, const gpgc_gdal_data& _dat, const float zeta, const int mu, bool max_error = true);


void gpgc_read(const char* filename, const int size);


std::array<std::vector<int>, 2> gpgc_decode_offsets(int* sizes, int num_sizes);

#endif
