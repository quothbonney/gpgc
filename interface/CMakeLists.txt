cmake_minimum_required(VERSION 3.4)
project(gpgc_pybind)

find_package(Python3 COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG)
find_package(GDAL REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

set (CMAKE_CXX_FLAGS "-lstdc++fs -std=c++17")
# create a library containing the source files and libraries
add_library(gpgclib STATIC ../../gpgcconv.cc ../../gpgclinearalgebra.c ../../gpgcencode.cc ../../gpgcutil.cc ../../gpgcdecode.cc)
target_link_libraries(gpgclib PRIVATE GDAL::GDAL Eigen3::Eigen ${PNG_LIBRARY})

# create the pybind11 module
pybind11_add_module(example gpgc_python_interface.cpp gpgc_python_implementation.cpp)
target_link_libraries(example PRIVATE gpgclib)

# create the testing binary
include_directories(${PYTHON_INCLUDE_DIRS})
add_executable(gpgc_python_test gpgc_interface_test.cpp)
target_link_libraries(gpgc_python_test PRIVATE gpgclib)
target_link_libraries(gpgc_python_test PRIVATE GDAL::GDAL ${PYTHON_LIBRARIES} Eigen3::Eigen ${PNG_LIBRARY})