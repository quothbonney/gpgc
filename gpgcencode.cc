#include "gpgc.hpp"
#include <cstdint>

#define GPGC_SKIP_BITSHIFTS 5

gpgc_partition::gpgc_partition(int _size, int _xoff, int _yoff, uint16_t** rasterBMP, gpgc_encoder* encoder_data, int _level)
        : size(_size), xOff(_xoff), yOff(_yoff) , bmp(rasterBMP) , level(_level) {
    if(gpgc_compression_paramters::num_nodes == 70) {
        std::cout << "test";
    }
    const int* _partition_block = get_block();
    const gpgc_vector fit = fit_vector(_partition_block);
    float entropy = get_entropy(fit, _partition_block);
    delete[] _partition_block;
    subpartition(entropy, encoder_data, &fit);
}

float gpgc_partition::get_entropy(const gpgc_vector& vec, const int* block) const {
	using namespace gpgc_compression_paramters;
    unsigned long long info = 0;
    for(size_t row = 0; row < size; ++row) {
        for(size_t cell = 0; cell < size; ++cell) {
            float expected = (vec.i * row) + (vec.j * cell) + vec.k; // In form ax_by_z for vector
            float cell_diff = expected - bmp[yOff + row][xOff + cell]; // Get difference
            if(gpgc_max_error != 0) {
                if(cell_diff > gpgc_max_error && size > 4)
                    return 32767.f;
            }
            float score = std::abs(cell_diff / gpgc_mu);
            float P_ak = inverse_z_transform(score);
            // Max to avoid negative info
            float point_info = fmax(0.0, -1 * std::log2(2.56*P_ak)); // Shannon info formula

            info += (long long)point_info;

        }
    }
    float adjusted_info = info / (size*size + 4);

    return adjusted_info;
}

void gpgc_partition::subpartition(float entropy, gpgc_encoder* _gpe, const gpgc_vector* _encoded_vector) {
	using namespace gpgc_compression_paramters;
    if(entropy > gpgc_zeta &&  size >= 4) {
        int new_size = size / 2;
        level++;
        gpgc_partition child1 = gpgc_partition(new_size, xOff, yOff, bmp, _gpe, level);
        gpgc_partition child2 = gpgc_partition(new_size, xOff + new_size, yOff, bmp, _gpe, level);
        gpgc_partition child3 = gpgc_partition(new_size, xOff, yOff + new_size, bmp, _gpe, level);
        gpgc_partition child4 = gpgc_partition(new_size, xOff + new_size, yOff + new_size, bmp, _gpe, level);
    } else {
		filled_size += size*size;
		num_nodes++;


        uint16_t encoded_int[4];
        memcpy(&encoded_int, _encoded_vector, sizeof(struct gpgc_vector));
        gpgc_encode_64(_gpe, encoded_int);
        gpgc_easy_write(_gpe, *_encoded_vector, size);
    
		if(_gpe->p % 10) {
			float prog = (float)gpgc_compression_paramters::filled_size / gpgc_compression_paramters::raster_size;
			print_progress(prog);
		}
        std::cout << "\nEncoded leaf node with size " << size << " at " << xOff << " " << yOff <<  ". Entropy=" << entropy << "\033[A\r";
    }
}

float* gpgc_partition::get_block_float() const {
    int sq = size * size;
    auto* block = new float[sq];

    for(size_t row = 0; row < size; ++row) {
        for(size_t cell = 0; cell < size; ++cell)
            block[(row * size) + cell] = bmp[row+yOff][cell + xOff];
    }
    return block;
}

int* gpgc_partition::get_block() const {
    int sq = size * size;
    auto* block = new int[sq];

    for(size_t row = 0; row < size; ++row) {
        for(size_t cell = 0; cell < size; ++cell) {
            int val = bmp[row + yOff][cell + xOff];
            val > 1000 ? val = 0 : val = val;
            block[(row * size) + cell] = val;
        }
    }
    return block;
}

gpgc_vector gpgc_partition::fit_vector(const int* block) {
    int sq = size*size;
    int skipper = size > 32 ? size >> GPGC_SKIP_BITSHIFTS : 1;
    using namespace Eigen;
    int cols = pow(size / skipper, 2);
    int rows = 3;

    int* arr_A = gpgc_create_matrix_A(size, skipper, block);
    // Creates the initial matrix as the transpose (easier to send to Eigen map buffer)
    MatrixXi eigen_matrix_A = Map<Matrix<int, 3, Dynamic> >(arr_A, rows, cols);

    int* arr_B = gpgc_create_matrix_B(size, skipper, block);
    VectorXi eigen_vector_B = Map<VectorXi>(arr_B, cols);
    Eigen::MatrixXf f_mat = (eigen_matrix_A * eigen_matrix_A.transpose()).cast<float>();

    Eigen::Vector3f x = (eigen_matrix_A.transpose()).cast<float>().householderQr().solve((eigen_vector_B).cast<float>());

    using half_float::half;
    gpgc_vector short_vector{(half)x[0], (half)x[1], (u_int16_t)std::max(x[2],0.f), (uint16_t)level};

	delete arr_A;
	delete arr_B;

    return short_vector;
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


uint16_t** gpgc_read_DEM(const gpgc_gdal_data* rData) {
    auto** block = new uint16_t*[rData->height];
    for(int row = 0; row < rData->height; ++row) {
        block[row] = new uint16_t[rData->width];
        auto tmpCPLerror = rData->rBand->RasterIO(GF_Read, 0, row, rData->width,
                               1, block[row], rData->width, 1,
                               GDT_Int16, 0, 0);
    }
    return block;
}


int gpgc_encode(char* filename, char* out_filename, const gpgc_gdal_data& _dat, const float zeta, const int mu, bool max_error) {

	// Define our encoding object, set the initial pointer to be right after magic and header 
    gpgc_encoder gpe{
            (uint16_t*) malloc(GPGC_HEADER_SIZE + (_dat.height * _dat.width)),
            GPGC_HEADER_SIZE
    };

    gpgc_header_t magic_header {
			GPGC_MAGIC_SIGNATURE,	
            _dat.width,
            _dat.height,
			0
    };

	// Define the compression parameters in the static namespace to be accessed by the encoder
	// There may be a better method for this, but I don't know it
	
	gpgc_compression_paramters::gpgc_mu        = mu;
	// If max_error is true (1), define it to be equal to GPGC_MAX_ERROR
	gpgc_compression_paramters::gpgc_max_error = max_error * GPGC_MAX_ERROR;
	gpgc_compression_paramters::gpgc_zeta	   = zeta;
	gpgc_compression_paramters::num_nodes	   = 0;

	// Initialize mosaicing system fragments in std::vector to be accessed later and compressed individually

    std::stringstream fname;
    fname << std::filesystem::current_path().c_str() << "/" << out_filename;
    FILE* f = fopen(fname.str().c_str(), "wb");

    gpe.ez_enc.open(fname.str() + ".log");


    uint16_t** rasterBMP = gpgc_read_DEM(&_dat);

	gpgc_compression_paramters::raster_size = magic_header.height * magic_header.width;

    gpgc_partition(_dat.height, 0, 0, rasterBMP, &gpe, 0);
	magic_header.node_count = gpgc_compression_paramters::num_nodes;

	memcpy(gpe.bytestream, &magic_header, sizeof(struct gpgc_header_t));

    fwrite(gpe.bytestream, 1, 2*gpe.p, f);
    free(gpe.bytestream);
    gpe.ez_enc.close();

	return 0;
}



