#include <cstdint>
#include "gdal_priv.h"
#include <gdal.h>
#include <stdio.h>
#include "half.hpp"
#include <iostream>
#include <eigen3/Eigen/Dense>

#define	GPGC_ZETA 0.3
#define GPGC_HEADER_SIZE 128

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
} gpgc_encoder;

struct gpgc_partition {
	int size, xOff, yOff;
	
	uint16_t** bmp;

	gpgc_partition(int _size, int _xoff, int _yoff, uint16_t** rasterBMP, gpgc_encoder* encoder_data)
	: size(_size), xOff(_xoff), yOff(_yoff) , bmp(rasterBMP) {
		const float* _partition_block = get_block();
		std::cout << "Testing ";
        const gpgc_vector fit = fit_vector(_partition_block);
        float entropy = get_entropy(fit, _partition_block);
        std::cout << "SIZE: " << size << " OFFSET: " << xOff << " " << yOff <<  " ENT.: " << entropy << "\n";
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
		if(entropy > GPGC_ZETA && size >= 4) {
			int new_size1, new_size2;
			new_size1 = (int) size / 2;
			new_size2 = size - new_size1;

			gpgc_partition child1 = gpgc_partition(new_size1, xOff, yOff, bmp, _gpe);
			gpgc_partition child2 = gpgc_partition(new_size1, xOff + new_size1, yOff, bmp, _gpe);
			gpgc_partition child3 = gpgc_partition(new_size1, xOff, yOff + new_size1, bmp, _gpe);
			gpgc_partition child4 = gpgc_partition(new_size1, xOff + new_size1, yOff + new_size1, bmp, _gpe);
		} else {
            uint64_t encoded_int;
            memcpy(&encoded_int, _encoded_vector, sizeof(struct gpgc_vector));
            _gpe->bytestream[(_gpe->p)++] = (0xFFFF000000000000 & encoded_int) >> 48;
            _gpe->bytestream[(_gpe->p)++] = (0x0000FFFF00000000 & encoded_int) >> 32;
            _gpe->bytestream[(_gpe->p)++] = (0x00000000FFFF0000 & encoded_int) >> 16;
            _gpe->bytestream[(_gpe->p)++] = (0x000000000000FFFF & encoded_int);
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

void* gpgc_encode(const gpgc_gdal_data& _dat) {
    gpgc_encoder gpe{
            (unsigned char *) malloc(GPGC_HEADER_SIZE + (_dat.height * _dat.width)),
            0
    };

    uint16_t** rasterBMP = gpgc_read_16(&_dat);
    gpgc_partition(_dat.height, 0, 0, rasterBMP, &gpe);

    FILE* f = fopen("test.gpgc", "wb");
    fwrite(gpe.bytestream, 1, gpe.p, f);
    free(gpe.bytestream);
}


int main(int argc, char *argv[]) {
	gpgc_gdal_data dat =	process_file(argv[1]);
    gpgc_encode(dat);
}
