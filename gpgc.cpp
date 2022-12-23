#include <cstdint>
#include "gdal_priv.h"
#include <gdal.h>
#include <stdio.h>
#include "half.hpp"
#include <iostream>
#include <eigen3/Eigen/Dense>

typedef struct {
	half_float::half i, j, k;
} gpgc_vector;

typedef struct {
	uint16_t height, width;
	GDALRasterBand* rBand;
	double rHeaderData[6];
} gpgc_gdal_data;

struct gpgc_partition {
	int size, xOff, yOff;
	
	uint16_t** bmp;

	gpgc_partition(int _size, int _xoff, int _yoff, uint16_t** rasterBMP)
	: size(_size), xOff(_xoff), yOff(_yoff) , bmp(rasterBMP) {
		float* _partition_block = get_block();

		gpgc_vector fit = fit_vector(_partition_block);
		float* diffs = get_differences(fit, _partition_block);

	}

private:
	auto get_differences(gpgc_vector vec, float* block) -> float* {
		auto* diffs = new float[size*size];
		for(size_t row = 0; row < size; ++row) {
			for(size_t cell = 0; cell < size; ++cell) {
				float expected = vec.i * cell + vec.j * row + vec.k;
				float cell_diff = expected - block[(row * size) + cell];
				diffs[(row*size) + cell] = cell_diff;
			}
		}
		return diffs;
	}

	auto get_block() -> float* {
		int sq = size * size;
        auto* block = new float[sq];

		for(size_t row = 0; row < size; ++row) {
            for(size_t cell = 0; cell < size; ++cell)
                block[(row * size) + cell] = bmp[row+yOff][cell + xOff];
		}
		return block;
	}

	auto fit_vector(float* block) -> gpgc_vector {
		int sq = size*size;
		Eigen::Map<Eigen::VectorXf> block_vector(block, sq);
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
		gpgc_vector short_vector{(half)x[0], (half)x[1], (half)x[2]};
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




int main(int argc, char *argv[]) {
	gpgc_gdal_data dat =	process_file(argv[1]);
	uint16_t** rasterBMP = gpgc_read_16(&dat);
	gpgc_partition(dat.height, 0, 0, rasterBMP);
}
