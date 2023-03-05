//
// Created by quothbonney on 3/5/23.
//

#include <pybind11/pybind11.h>
#include "../gpgc.hpp"

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

int add(int i, int j) {
    return i + j;
}


PYBIND11_MODULE(example, m) {
    py::class_<gpgc_gdal_data>(m, "gpgc_gdal_data")
            .def("__repr__",
                 [](const gpgc_gdal_data &a) {
                     return "<gpgc_gdal_data>";
                 });

    py::class_<gpgc_encoder>(m, "gpgc_encoder")
            .def("__repr__",
                 [](const gpgc_encoder &a) {
                     return "<gpgc_encoder>";
                 });

    m.def("process_tif", [](char* filename) {
        return process_file(filename);
    });

    m.def("gpgc_encode", [](char* filename, gpgc_gdal_data _dat, const float zeta, const int mu, bool max_error) {
        return gpgc_encode_bytes(filename, _dat, zeta, mu, max_error);
    });
}