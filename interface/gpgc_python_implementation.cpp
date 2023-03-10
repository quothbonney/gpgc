//
// Created by Jack Carson on 3/5/23.
//


#include "gpgc_python_interface.hpp"

namespace py = pybind11;


gpgc_encoder gpgc_encode_bytes(char* filename, const gpgc_gdal_data& _dat, const float zeta, const int mu, bool max_error) {

    // Define our encoding object, set the initial pointer to be right after magic and header
    gpgc_encoder gpe{
            (char*) malloc(GPGC_HEADER_SIZE + (_dat.height * _dat.width)),
            0
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


    uint16_t** rasterBMP = gpgc_read_DEM(&_dat);

    gpgc_compression_paramters::raster_size = magic_header.height * magic_header.width;

    gpgc_partition(_dat.height, 0, 0, rasterBMP, &gpe, 0);
    magic_header.node_count = gpgc_compression_paramters::num_nodes;

    free(rasterBMP);

    return gpe;
}

py::tuple node_vector_to_tuple(const gpgc_vector& p) {
    float i = p.i;
    float j = p.j;
    return py::make_tuple(i, j, p.k, p.size);
}

std::vector<py::tuple> gpgc_read_to_python(const gpgc_encoder& _gpe) {
    int p_size = _gpe.p / sizeof(struct gpgc_vector);

    std::vector<py::tuple> dc_vectors;
    for(int i = 0; i < p_size; ++i) {
        int position = i * sizeof(struct gpgc_vector);
        gpgc_vector *v = new gpgc_vector;
        memcpy(v, &_gpe.bytestream[position], sizeof(struct gpgc_vector));
        dc_vectors.push_back(node_vector_to_tuple(*v));
        std::cout << v->i << " " << v->j << " " << v-> k << " " << v->size << "\n";
    }

    return dc_vectors;
}

py::array_t<double> decoded_vectors_to_numpy(const std::vector<py::tuple>& decoded_vectors) {
    auto size = decoded_vectors.size();
    auto numpy_array = py::array_t<double>(size);
    auto numpy_array_data = numpy_array.mutable_data();

    for(size_t i = 0; i < size; ++i) {
        auto tuple = decoded_vectors[i];
        numpy_array_data[i] =
                py::cast<double>(tuple[0]) +
                py::cast<double>(tuple[1]) +
                py::cast<double>(tuple[2]) +
                py::cast<double>(tuple[3]);
    }
    return numpy_array;
}

