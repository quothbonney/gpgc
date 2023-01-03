#ifndef GPGC_H
#define GPGC_H

#include <cstdint>
#include "gdal_priv.h"
#include <gdal.h>
#include <ostream>
#include <stdio.h>
#include "half/half/half.hpp"
#include <iostream>
#include <eigen3/Eigen/Dense>
#include <fstream>

#define	GPGC_ZETA 0.3
#define GPGC_HEADER_SIZE 8 

struct gpgc_header_t {
	uint32_t width;
	uint32_t height;
};

struct gpgc_vector{
	half_float::half i, j;
    int16_t k;
    u_int16_t size;
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

struct gpgc_partition {
	int xOff, yOff, size;

	uint16_t** bmp;

	gpgc_partition(int _size, int _xoff, int _yoff, uint16_t** rasterBMP, gpgc_encoder* encoder_data)
	: size(_size), xOff(_xoff), yOff(_yoff) , bmp(rasterBMP) {
		const float* _partition_block = get_block();
        const gpgc_vector fit = fit_vector(_partition_block);
        float entropy = get_entropy(fit, _partition_block);
        delete[] _partition_block;
        subpartition(entropy, encoder_data, &fit);
	}

private:

	float get_entropy(const gpgc_vector& vec, const float* block) const {
		unsigned long info = 0;
		for(size_t row = 0; row < size; ++row) {
			for(size_t cell = 0; cell < size; ++cell) {
				float expected = (vec.i * row) + (vec.j * cell) + vec.k; // In form ax_by_z for vector
				float cell_diff = expected - bmp[yOff + row][xOff + cell]; // Get difference
				float score = std::abs(cell_diff / 30); // Assuming sigma = 30 for z score, arbitrary
				float P_ak = 1 / std::pow(4.0, score); // Arbitrary formula TODO: update
				float point_info = -1 * std::log2(P_ak); // Shannon info formula

				info += point_info;
			}
		}
		float adjusted_info = (long double)info / (size*size);

		return adjusted_info;
	}

	void subpartition(float entropy, gpgc_encoder* _gpe, const gpgc_vector* _encoded_vector) {
		if(entropy > GPGC_ZETA &&  size >= 4) {
			int new_size = size / 2;

			gpgc_partition child1 = gpgc_partition(new_size, xOff, yOff, bmp, _gpe);
			gpgc_partition child2 = gpgc_partition(new_size, xOff + new_size, yOff, bmp, _gpe);
			gpgc_partition child3 = gpgc_partition(new_size, xOff, yOff + new_size, bmp, _gpe);
			gpgc_partition child4 = gpgc_partition(new_size, xOff + new_size, yOff + new_size, bmp, _gpe);
		} else {
			std::cout << "Encoded leaf node with size " << size << " at " << xOff << " " << yOff <<  ". Entropy=" << entropy << "\n";

            uint64_t encoded_int;
            memcpy(&encoded_int, _encoded_vector, sizeof(struct gpgc_vector));
			gpgc_encode_64(_gpe, encoded_int);
			gpgc_easy_write(_gpe, *_encoded_vector, size);
		}
	}

	auto get_block() -> float* const {
		int sq = size * size;
        auto* block = new float[sq];

		for(size_t row = 0; row < size; ++row) {
            for(size_t cell = 0; cell < size; ++cell)
                block[(row * size) + cell] = bmp[row+yOff][cell + xOff];
		}
		return block;
	}

	auto fit_vector(const float* block) -> gpgc_vector {
		int sq = size*size;
		Eigen::Map<Eigen::VectorXf> block_vector(const_cast<float*>(block), sq);
		Eigen::VectorXi v(sq), a1(sq), a2(sq), a3(sq);

		v = Eigen::VectorXi::LinSpaced(sq, 0, sq-1);
		a1 = v.unaryExpr([&](const int x) { return x % size; });
		a2 = v / size;
		a3.setConstant(1);

		Eigen::MatrixXi m(sq, 3);
		m << a1, a2, a3;
		Eigen::MatrixXf f = m.cast<float>();

		Eigen::ColPivHouseholderQR<Eigen::MatrixXf> dec(f);  // Convienent Eigen function to solve for x-bar vector. Saves difficult matrix algebra with LU algorithm
        Eigen::Vector3f x = dec.solve(block_vector);
		using half_float::half; 
		gpgc_vector short_vector{(half)x[0], (half)x[1], (int16_t)x[2]};
		return short_vector;
	}

};

void gpgc_encode_64(gpgc_encoder* _gpe, const uint64_t& serialized) {
	_gpe->bytestream[(_gpe->p)++] = (0xFFFF000000000000 & serialized) >> 48;
	_gpe->bytestream[(_gpe->p)++] = (0x0000FFFF00000000 & serialized) >> 32;
	_gpe->bytestream[(_gpe->p)++] = (0x00000000FFFF0000 & serialized) >> 16;
	_gpe->bytestream[(_gpe->p)++] = (0x000000000000FFFF & serialized);
}

void gpgc_easy_write(gpgc_encoder* _gpe, gpgc_vector fit, int size) {
	_gpe->ez_enc << fit.i << " " << fit.j << " " << fit.k << " " << size << "\n";
}


gpgc_gdal_data process_file(const char* filename) {
	GDALAllRegister();
	GDALDataset* poDataset;
	gpgc_gdal_data working;

	poDataset = (GDALDataset*)GDALOpen(filename, GA_ReadOnly);
	working.rBand = poDataset->GetRasterBand(1);

	working.width = working.rBand->GetXSize();
	working.height= working.rBand->GetYSize();
	poDataset->GetGeoTransform(working.rHeaderData);

    return working;
}

uint16_t** gpgc_read_16(const gpgc_gdal_data* rData) {
	auto** block = new uint16_t*[rData->height];
	for(int row = 0; row < rData->height; ++row) { 
		block[row] = new uint16_t[rData->width];
		rData->rBand->RasterIO(GF_Read, 0, row, rData->width,
								1, block[row], rData->width, 1, 
								GDT_Int16, 0, 0);
	}
	return block;
}

void* gpgc_encode(char* filename, const gpgc_gdal_data& _dat) {
    gpgc_encoder gpe{
            (unsigned char *) malloc(GPGC_HEADER_SIZE + (_dat.height * _dat.width)),
            0
    };
	gpe.ez_enc.open("./output.txt");

	gpgc_header_t magic_header {
		_dat.width,
		_dat.height
	};

    uint16_t** rasterBMP = gpgc_read_16(&_dat);

	uint64_t serialized_header;
	memcpy(&serialized_header, &magic_header, sizeof(struct gpgc_header_t));

	gpgc_encode_64(&gpe, serialized_header);
    gpgc_partition(_dat.height, 0, 0, rasterBMP, &gpe);

    FILE* f = fopen("test.gpgc", "wb");
    fwrite(gpe.bytestream, 1, gpe.p, f);
    free(gpe.bytestream);
	gpe.ez_enc.close();
}

#endif
