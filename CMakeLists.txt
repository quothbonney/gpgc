cmake_minimum_required(VERSION 3.14)
project(gpgc C CXX)

find_package(GDAL REQUIRED)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -pthread")
if(GDAL_FOUND)
    include_directories(${GDAL_INCLUDE_DIR})
else(GDAL_FOUND)
    message(WARNING "GDAL NOT FOUND. Skipping GDAL package build...")
endif()
add_compile_options(-g)
add_executable(gpgcconv gpgcconv.cc gpgclinearalgebra.c gpgcencode.cc gpgcutil.cc gpgcdecode.cc)

target_link_libraries(gpgcconv PRIVATE GDAL::GDAL Eigen3::Eigen)

target_compile_features(gpgcconv PRIVATE cxx_std_17)

