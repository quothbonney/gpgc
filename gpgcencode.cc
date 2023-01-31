#include "gpgc.hpp"

#define GPGC_SKIP_BITSHIFTS 4

gpgc_partition::gpgc_partition(int _size, int _xoff, int _yoff, uint16_t** rasterBMP, gpgc_encoder* encoder_data)
        : size(_size), xOff(_xoff), yOff(_yoff) , bmp(rasterBMP) {
    const float* _partition_block = get_block();
    const gpgc_vector fit = fit_vector(_partition_block);
    float entropy = get_entropy(fit, _partition_block);
    delete[] _partition_block;
    subpartition(entropy, encoder_data, &fit);
}

float gpgc_partition::get_entropy(const gpgc_vector& vec, const float* block) const {
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
            float point_info = -1 * std::log2(2.56*P_ak); // Shannon info formula

            info += point_info;
        }
    }
    float adjusted_info = (long double)info / (size*size + 4);

    return adjusted_info;
}

void gpgc_partition::subpartition(float entropy, gpgc_encoder* _gpe, const gpgc_vector* _encoded_vector) {
	using namespace gpgc_compression_paramters;
    if(entropy > gpgc_zeta &&  size >= 4) {
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

float* gpgc_partition::get_block() const {
    int sq = size * size;
    auto* block = new float[sq];

    for(size_t row = 0; row < size; ++row) {
        for(size_t cell = 0; cell < size; ++cell)
            block[(row * size) + cell] = bmp[row+yOff][cell + xOff];
    }
    return block;
}

int* gpgc_partition::get_block_int() const {
    int sq = size * size;
    auto* block = new int[sq];

    for(size_t row = 0; row < size; ++row) {
        for(size_t cell = 0; cell < size; ++cell)
            block[(row * size) + cell] = bmp[row+yOff][cell + xOff];
    }
    return block;
}

gpgc_vector gpgc_partition::fit_vector(const float* block) {
    int sq = size*size;
    int skipper = size > 32 ? size >> GPGC_SKIP_BITSHIFTS : 1;
    using namespace Eigen;
    int cols = pow(size / skipper, 2) + 1;
    int rows = 3;

    int* arr_A = gpgc_create_eigen_A(size, skipper, block);
    // Creates the initial matrix as the transpose (easier to send to Eigen map buffer)
    MatrixXi eigen_matrix_A = Map<Matrix<int, Dynamic, Dynamic> >(arr_A, rows, cols);
    std::cout << eigen_matrix_A.transpose();

    int* matrix_B = gpgc_create_matrix_B(size, skipper, block);

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
        auto tmpCPLerror = rData->rBand->RasterIO(GF_Read, 0, row, rData->width,
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

void gpgc_encode(char* filename, char* out_filename, const gpgc_gdal_data& _dat, const float zeta, const int mu, bool max_error) {
    gpgc_encoder gpe{
            (unsigned char *) malloc(GPGC_HEADER_SIZE + (_dat.height * _dat.width)),
            0
    };

	gpgc_compression_paramters::gpgc_max_error = max_error * 50;
	gpgc_compression_paramters::gpgc_mu        = mu;
	gpgc_compression_paramters::gpgc_zeta	   = zeta;
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

void gpgc_read(const char* filename, const int size) {
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
	std::array<std::vector<int>, 2> p;
	return p;
}

