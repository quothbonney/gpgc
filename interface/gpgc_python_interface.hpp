//
// Created by quothbonney on 3/6/23.
//

#ifndef GPGC_PYBIND_GPGC_PYTHON_INTERFACE_HPP
#define GPGC_PYBIND_GPGC_PYTHON_INTERFACE_HPP


#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include "../gpgc.hpp"

gpgc_encoder gpgc_encode_bytes(char* filename, const gpgc_gdal_data& _dat, const float zeta, const int mu, bool max_error);

std::vector<pybind11::tuple> gpgc_read_to_python(const gpgc_encoder& _gpe);

pybind11::tuple node_vector_to_tuple(const gpgc_vector& p);

pybind11::array_t<double> decoded_vectors_to_numpy(const std::vector<pybind11::tuple>& decoded_vectors);
#endif //GPGC_PYBIND_GPGC_PYTHON_INTERFACE_HPP
