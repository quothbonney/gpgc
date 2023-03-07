

PYBIND11_MODULE(example, m) {
    py::class_<gpgc_vector>(m, "gpgc_vector")
            .def(py::init<half_float::half, half_float::half, int, int>())
            .def_readwrite("i", &gpgc_vector::i)
            .def_readwrite("j", &gpgc_vector::j)
            .def_readwrite("k", &gpgc_vector::k)
            .def_readwrite("size", &gpgc_vector::size);

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

    m.def("gpgc_decode", [](const gpgc_encoder& _gpe) {
       auto vec = gpgc_read_to_python(_gpe);
       return decoded_vectors_to_numpy(vec);
    });
}