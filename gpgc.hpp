/*
 * Copyright (c) 2023, Jack David Carson - https://github.com/quothbonney
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
#include <png.h>
#include "half/half/half.hpp"
#include "gpgcmath.h"
#include "gpgcutil.hpp"
#include "gdal_priv.h"


/*
 * GPGC header has not yet been specified. This is reserved for a future implementation,
 * however the size is parametrically implemented already and can be added easily with
 * the geotiffio reaster reader. Defined as 8 bytes to fit 2 32-bit integers defined in
 * gpgc_header_t
 */

#define GPGC_HEADER_SIZE 16 

#define GPGC_MAGIC_SIGNATURE 0x67706763 // ASCII -> Hex for "GPGC"
#define GPGC_MAGIC_DECIMAL 1735419747


/*
 * These parameters are passed from the implementation functions to this namespace
 * where they are defined as static variables. It is best not to interact with this 
 * namespace directly.
 *
 *		gpgc_max_error: The maximum error acceptable before a forced partition. This can be set to undefined.
 *		gpgc_mu:		The threshold for acceptable average entropy of an encoded partition
 *		gpgc_zeta:		Standard deviation of average errors; used to calculate information entropy
 */

#define GPGC_MAX_ERROR 50

namespace gpgc_compression_paramters {
	static int gpgc_max_error;
	static int gpgc_mu;
	static float gpgc_zeta;
	
	// Used for progress bar
	static long raster_size;
	static long filled_size;

	static int num_nodes;
}

/*
 * Header for GPGC files providing information about how to read, size, bands, and location metadata
 * stripped from GeoTIFF header. Anticipating geotiffio implementation.
 */

struct gpgc_header_t {
	uint32_t magic;

	uint32_t width;
	uint32_t height;

	uint32_t node_count;
};

/*
 * Key leaf node structure, containing two half precision 16-bit floats that are defined in submodule
 * half. k-hat and size are 16-bit unsigned integers. Overall the struct is 64 bits serialized and encoded.
 */
struct gpgc_vector{
	half_float::half i, j;
    u_int16_t k;
    u_int8_t size;
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
   char* bytestream;
   int p;
   std::ofstream ez_enc;
} gpgc_encoder;


/* 
 * Encode 64 bit unsigned into the gpgc_encoder struct. Used to pass data to the byte stream
 * that is subsequently encoded. Whatever is passed ot the unint64 must be already serialized
 * as desired. Primarily used for encoding of `gpgc_header_t` and the `gpgc_vector` nodes that
 * form the tree structure in the bytestram.
 */
void gpgc_encode_64(gpgc_encoder *_gpe, const uint64_t &serialized);

/*
 * Used with std::ofstream ez_enc to output data to a .gpgc.log file that is easily readable without
 * binary unpacking (used in py/ scripts)
 */
void gpgc_easy_write(gpgc_encoder *_gpe, gpgc_vector fit, int size);

/*
 * Object for each partition used to hold information about its location in the raster
 */
struct gpgc_partition {
	int xOff, yOff, size, level;

	uint16_t** bmp; // Raw raster data passed by pointer for use

	gpgc_partition(int _size, int _xoff, int _yoff, uint16_t** rasterBMP, gpgc_encoder* encoder_data, int _level);

private:
	// Returns area for partition in a concatenated int*
	int* get_block() const;

	float* get_block_float() const;

	// Fits int* block to gpgc_vector using Eigen decomposisition solver
	gpgc_vector fit_vector(const int* block);
	
	// Calculates entropy per point in block, returns as average value
	float get_entropy(const gpgc_vector& vec, const int* block) const;

	// Decides whether to partition based on entropy value. If no partition, it is encoded into the bytestream
	void subpartition(float entropy, gpgc_encoder* _gpe, const gpgc_vector* _encoded_vector);
};

/*
 * Shifts the serialized data into the bytestream at position p
 * Relies on data to be 64-bit, as it is bitshifted into 4 16 bit fragments.
 * Bitwise AND for each two bytes of the serialized vector which are then bitshifted into the correct place
 */
inline void gpgc_encode_64(gpgc_encoder* _gpe, const gpgc_vector* serialized_vector) {
    memcpy(&_gpe->bytestream[(_gpe->p)], serialized_vector, sizeof(struct gpgc_vector));
    _gpe->p += 7;
    /*
	_gpe->bytestream[(_gpe->p) += 2] = (serialized_vector[0]);
	_gpe->bytestream[(_gpe->p) += 2] = (serialized_vector[1]);
	_gpe->bytestream[(_gpe->p) += 2] = (serialized_vector[2]);
	_gpe->bytestream[(_gpe->p)++] = lvl;
     */
}

/*
 * NOT IMPLEMENTED YET
 * Indicator of new tree structure in 24 bytes.`
 */
inline void gpgc_encode_size(gpgc_encoder* _gpe, const uint8_t iteration_size) {
    _gpe->bytestream[(_gpe->p)++] = (0xFFFF & iteration_size);
    _gpe->bytestream[(_gpe->p)++] = (0x737A) >> 8; // Hints at next character being size
}

/*
 * Simplified UTF-8 encoding into `.gpgc.log` for easy unpacking in Python
 */
inline void gpgc_easy_write(gpgc_encoder* _gpe, gpgc_vector fit, int size) {
    _gpe->ez_enc << fit.i << " " << fit.j << " " << fit.k << " " << size << "\n";
}

/*
 * Same UTF-8 encoding for `gpgc_encode_size` method
 */
inline void gpgc_easy_size(gpgc_encoder* _gpe, int size, int num) {
    _gpe ->ez_enc << "SIZEIS: " << size << " " << num << "\n";
}

/*
 * Uses GDAL to grab information from GeoTIFF/DTED file header
 * Returns gpgc_gdal_data with relevant geotransformdata
 */
gpgc_gdal_data process_file(const char* filename);

/*
 * Using GDAL RasterBand* abstraction to read raw 16-bit integers
 * from DEM input and return double int pointer. Double pointer forms BMP object that all partitions reference.
 * Looking to be deprecated with custom TIFF reader to work for embedded systems
 */
uint16_t** gpgc_read_DEM(const gpgc_gdal_data* rData);

/*
 * Returns floor of log2. Used heavily in decoding to reconstruct sizes
 */
inline int maxl2(int size) {
    return pow(2, std::floor(std::log2(size)));
}

/*
 * Encodes data from an input DEM file into the .GPGC specification. The function takes two char* for input and output filenames,
 * GDAL header data to be passed into the new header, as well as optional global compression parameters zeta, mu, and max_error, which
 * define respectively the maximum entropy, the standard deviation of residuals, and the option to consider the maximum residual.
 *
 * The function will create a `.gpgc` and `.gpgc.log` file in its attempt to compress the data. If the encoding was successful, it will
 * return 0, otherwise it will return 1.
 */
int gpgc_encode(char* filename, char* out_filename, const gpgc_gdal_data& _dat, const float zeta, const int mu, bool max_error = true);

/*
 * Decodes binary in filename to form reconstructed pointer array of gpgc_vectors needed for re-rasterization. Function manually does Bitwise AND
 * and bitshifts from a 64 bit object; tests are needed to validate this, but for now it works. Skips `sizeof(gpgc_header_t)` bits in the encoded file and
 * encodes them into the `gpgc_header_t` pointer structure passed by pointer to the function.
 */
gpgc_vector* gpgc_read(const char* filename, gpgc_header_t* head);

/*
 * Relies on data provided by the `gpgc_read()` function to supply vectors and sizes. Fills two arrays x0, y0 with the offsets for
 * each vector. Uses queue-based system for ultra-fast reconstruction of a depth first quadtree search for positioning in two
 * dimensions. Header must be passed for memory reservations by `header.node_count`
 */
int gpgc_decode_offsets(gpgc_vector* dc_vectors, const gpgc_header_t& header, std::vector<float>& x0, std::vector<float>& y0);

/*
 * Finally fills a datablock with reconstructed partitions by the orthogonal vector provided in gpgc_read() at offset
 * calculated with gpgc_decode_offsets. Once the datablock is created, it is up to the user what is to be done with it. Relies
 * on the same parameters as gpgc_decode_offsets().
 */
int** gpgc_reconstruct(gpgc_vector* dc_vectors, const gpgc_header_t& header, std::vector<float>& x0, std::vector<float>& y0);

/*
 * Saves data in PNG format. Looking to move to gpgcutil.hpp
 */
bool save_png(const char* filename, int** image, int width, int height);
#endif
