/*
 * Copyright (c) 2023, Jack Carson - https://github.com/quothbonney
 * SPDX-License-Identifier: MIT
 *
 *
 * The "General Purpose Geospatial Compression" format for high efficiency lossy compression 
 *
 * -- About
 *
 *  GPGC encodes data based on a set-partition-code algorithm, representing GeoTIFF 
 *  raster files as a quadtree structure of orthogonal vectors. GPGC offers compression
 *  ratios from 10 to 70, with linear decoding, and procedurally ensured quality and 
 *  accuracy guaruntees.
 *
 *
 *  -- Usage
 *  
 *  Ensure you `#define GPGC_IMPLEMENTATION` in one C/CXX file before including the `.hpp` file
 *			   `#include "gpgc.hpp"`
 */


#ifndef GPGC_H
#define GPGC_H

#include <cstdint>
#include <ostream>
#include <iostream>
#include <string>
#include <eigen3/Eigen/Dense>
#include <fstream>
#include <filesystem>
#include <sys/types.h>
#include "half/half/half.hpp"
#include "gpgcmath.h"
#include "gdal_priv.h"


/*
 * GPGC header has not yet been specified. This is reserved for a future implementation,
 * however the size is parametrically implemented already and can be added easily with
 * the geotiffio reaster reader. Defined as 8 bytes to fit 2 32-bit integers defined in
 * gpgc_header_t
 */

#define GPGC_HEADER_SIZE 8 

/*
 * These parameters are passed from the implementation functions to this namespace
 * where they are defined as static variables. It is best not to interact with this 
 * namespace directly.
 *
 *		gpgc_max_error: The maximum error acceptable before a forced partition. This can be set to undefined.
 *		gpgc_mu:		The threshold for acceptable average entropy of an encoded partition
 *		gpgc_zeta:		Standard deviation of average errors; used to calculate information entropy
 */

namespace gpgc_compression_paramters {
	static int gpgc_max_error;
	static int gpgc_mu;
	static float gpgc_zeta;
}

/*
 * Header for GPGC files providing information about how to read, size, bands, and location metadata
 * stripped from GeoTIFF header. Anticipating geotiffio implementation.
 */

struct gpgc_header_t {
	uint32_t width;
	uint32_t height;
};

/*
 * Key leaf node structure, containing two half precision 16-bit floats that are defined in submodule
 * half. k-hat and size are 16-bit unsigned integers. Overall the struct is 64 bits serialized and encoded.
 */
struct gpgc_vector{
	half_float::half i, j;
    int16_t k;
    u_int16_t size;
};

/*
 * Structure containing information about mosaic fragment locations
 */
struct raster_offset {
    int x, y, size;

    raster_offset(int _x,int _y,int _s) : x(_x), y(_y), size(_s) {};
};

/*
 * Header data from GeoTIFF as provided by GDAL. Current use: rBand->rasterIO outputs int** buffer for
 * raw compression data. rHeaderData[6] contains header location data identifying the raster in space. 
 * Height and width passed to `gpgc_header_t`. Looking to remove dependency and structure.
 */
typedef struct {
	uint16_t height, width;
	GDALRasterBand* rBand;
	double rHeaderData[6];
} gpgc_gdal_data;

typedef struct {
	unsigned int width;
	unsigned int height;
} gpgc_desc;

/*
 * Container for encoding information passed by pointer to every object. `bytestream` holds the encoded
 * that is assigned to the output `.gpgc` file. p is the position in the bytestream the data needs to be
 * entered. ez_enc is for python `.gpgc.log` output, allowing for UTF-8 unpackinig and simple analysis.
 */
typedef struct {
   unsigned char* bytestream;
   int p;
   std::ofstream ez_enc;
} gpgc_encoder;

void gpgc_encode_64(gpgc_encoder *_gpe, const uint64_t &serialized);
void gpgc_easy_write(gpgc_encoder *_gpe, gpgc_vector fit, int size);
double inverse_z_transform(double z_score);


struct gpgc_partition {
	int xOff, yOff, size;
	uint16_t** bmp;
	gpgc_partition(int _size, int _xoff, int _yoff, uint16_t** rasterBMP, gpgc_encoder* encoder_data);

private:
	float get_entropy(const gpgc_vector& vec, const float* block) const;

	void subpartition(float entropy, gpgc_encoder* _gpe, const gpgc_vector* _encoded_vector);

	float* get_block() const;

	int* get_block_int() const;

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
