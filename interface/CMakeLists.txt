cmake_minimum_required(VERSION 3.4)
project(gpgc_pybind)

find_package(Python3 COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG)

find_package(GDAL REQUIRED)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)

pybind11_add_module(example gpgc_python_interface.cpp gpgc_python_implementation.cpp  ../../gpgcconv.cc ../../gpgclinearalgebra.c ../../gpgcencode.cc ../../gpgcutil.cc ../../gpgcdecode.cc)

add_compile_options(-g)
add_custom_target(python_tests
        SOURCES gpgc_interface_test.cpp)
target_link_libraries(example PRIVATE GDAL::GDAL Eigen3::Eigen ${PNG_LIBRARY})
target_compile_features(example PRIVATE cxx_std_17)
