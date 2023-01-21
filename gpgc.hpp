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

int gpgc_max_error;

int gpgc_mu;
int gpgc_zeta;

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
int** gpgc_create_matrix_A(int partition_size);

struct gpgc_partition {
	int xOff, yOff, size;
	uint16_t** bmp;
	gpgc_partition(int _size, int _xoff, int _yoff, uint16_t** rasterBMP, gpgc_encoder* encoder_data);

private:
	float get_entropy(const gpgc_vector& vec, const float* block) const;

	void subpartition(float entropy, gpgc_encoder* _gpe, const gpgc_vector* _encoded_vector);

	float* get_block() const;

	gpgc_vector fit_vector(const float* block) {
		int sq = size*size;
		gpgc_create_matrix_A(sq);
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

inline void gpgc_encode_64(gpgc_encoder* _gpe, const uint64_t& serialized) {
	_gpe->bytestream[(_gpe->p)++] = (0xFFFF000000000000 & serialized) >> 48;
	_gpe->bytestream[(_gpe->p)++] = (0x0000FFFF00000000 & serialized) >> 32;
	_gpe->bytestream[(_gpe->p)++] = (0x00000000FFFF0000 & serialized) >> 16;
	_gpe->bytestream[(_gpe->p)++] = (0x000000000000FFFF & serialized);

}

void gpgc_decode_64(gpgc_encoder* _gpe) {
	unsigned int a = _gpe->bytestream[(_gpe->p)++];
}
	
void gpgc_encode_size(gpgc_encoder* _gpe, const uint8_t iteration_size) {
	_gpe->bytestream[(_gpe->p)++] = (0xFFFF & iteration_size);
	_gpe->bytestream[(_gpe->p)++] = (0x737A) >> 8; // Hints at next character being size
}

void gpgc_easy_write(gpgc_encoder* _gpe, gpgc_vector fit, int size) {
	_gpe->ez_enc << fit.i << " " << fit.j << " " << fit.k << " " << size << "\n";
}

void gpgc_easy_size(gpgc_encoder* _gpe, int size, int num) {
	_gpe ->ez_enc << "SIZEIS: " << size << " " << num << "\n";
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

inline double inverse_z_transform(double z_score) {
	auto inverse_function = [](double z) -> double {
		double pi = 3.1415;
		return (1 / std::sqrt(2*pi)) * std::exp(-(z*z)/2);
	};

	double upper = inverse_function(z_score);

	return upper;
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

inline int maxl2(int size) {
	return pow(2, std::floor(std::log2(size)));
}

std::vector<raster_offset> iteration_map(int raster_size, int it_size, int offX, int offY) {
	static std::vector<raster_offset> iterations;
    if(offX >= raster_size - 1 || offY >= raster_size - 1)
        return iterations;

	if(offX + it_size == raster_size || offY + it_size == raster_size) {
        iterations.emplace_back(offX, offY, it_size);
        return iterations;
    } else if(offX + it_size < raster_size && offY + it_size < raster_size) {
        int new_size = it_size;
        iterations.emplace_back(offX, offY, it_size);

        iteration_map(raster_size, new_size, offX + it_size, offY);
        iteration_map(raster_size, new_size, offX, offY + it_size);
        iteration_map(raster_size, new_size, offX + it_size, offY + it_size);
    } else {
        int new_size = it_size / 2;
        iteration_map(raster_size, new_size, offX, offY);
        iteration_map(raster_size, new_size, offX + new_size, offY);
        iteration_map(raster_size, new_size, offX, offY + new_size);
        iteration_map(raster_size, new_size, offX + new_size, offY + new_size);
    }


	return iterations;
}

void* gpgc_encode(char* filename, char* out_filename, const gpgc_gdal_data& _dat, const float zeta, const int mu, bool max_error = true) {
    gpgc_encoder gpe{
            (unsigned char *) malloc(GPGC_HEADER_SIZE + (_dat.height * _dat.width)),
            0
    };
	
	gpgc_max_error = max_error * 50;
	gpgc_mu = mu;
	gpgc_zeta = zeta;

	std::vector<raster_offset> iterations = iteration_map(_dat.width, maxl2(_dat.width), 0, 0);

	std::stringstream fname;
	fname << std::filesystem::current_path().c_str() << "/" << out_filename;
    FILE* f = fopen(fname.str().c_str(), "wb");
	
	gpe.ez_enc.open(fname.str() + ".log");

	gpgc_header_t magic_header {
		_dat.width,
		_dat.height
	};

    uint16_t** rasterBMP = gpgc_read_16(&_dat);

	uint64_t serialized_header;
	memcpy(&serialized_header, &magic_header, sizeof(struct gpgc_header_t));

	gpgc_encode_64(&gpe, serialized_header);

    for(int sz_i = 0; sz_i < iterations.size(); ++sz_i) {
		if(iterations[sz_i].size != iterations[sz_i-1].size) {
			size_t extra = 0;
			while(iterations[sz_i + extra].size == iterations[sz_i].size) extra++;
			//gpgc_encode_size(&gpe, std::log2(iterations[sz_i].size));
			gpgc_easy_size(&gpe, iterations[sz_i].size, extra);
		}
        gpgc_partition(iterations[sz_i].size, iterations[sz_i].x, iterations[sz_i].y, rasterBMP, &gpe);
    }
    fwrite(gpe.bytestream, 1, gpe.p, f);
    free(gpe.bytestream);
	gpe.ez_enc.close();
}

void* qoi_decode(const void* data, int f_size, int s_size) {
	const unsigned char *bytes;
	bytes = (const unsigned char*)bytes;


}

void* gpgc_read(const char* filename, const int size) {
	std::ifstream gpgc_file(filename, std::ios::binary);
	int x;
    while (gpgc_file.read(reinterpret_cast<char*>(&x), sizeof(u_int16_t))) {
        std::cout << (u_int16_t)x << std::endl;
    }

    gpgc_file.close();
}


std::array<std::vector<int>, 2> gpgc_decode_offsets(int* sizes, int num_sizes) {
	std::vector<int> x0 = { 0 };
	std::vector<int> y0 = { 0 };
	std::vector<int> b  = { 0 };
	
	auto it_b = b.begin();
	auto it_y = y0.begin();
	auto it_x = x0.begin();
	size_t index = 0;
	while(index < num_sizes) {
		while(b[index] < sizes[index]) {
			b[index] = b[index] + 1;
			for(int i = 0; i < 3; ++i) 
				b.insert(it_b + index + 1, b[index]);

            x0.insert(it_x + index + 1, x0[index] + (1 / pow(2, b[index])));
            x0.insert(it_x + index + 2, x0[index]);
            x0.insert(it_x + index + 3, x0[index] + (1 / pow(2, b[index])));

            y0.insert(it_y + index + 1, y0[index]);
            y0.insert(it_y + index + 2, y0[index] + (1 / pow(2, b[index])));
            y0.insert(it_y + index + 3, y0[index] + (1 / pow(2, b[index])));
		}
		index++;
	}
}

#endif
