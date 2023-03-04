cmake_minimum_required(VERSION 3.14)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${gpgc_SOURCE_DIR}/bin)
project(tests)
include(FetchContent)
set(BUILD_SHARED_LIBS OFF)

# -----------------------------TESTING-------------------------------
# adds ghs_filesystem Catch2::Catch2 ApprovalTests::ApprovalTests fmt::fmt

FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v2.13.4)
FetchContent_MakeAvailable(Catch2)

find_package(GDAL REQUIRED)

add_executable(tests
        ../gpgcdecode.cc
        ../gpgcencode.cc
        ../gpgcutil.cc
        ../gpgclinearalgebra.c
        ../gpgc.hpp
        main.cpp
)
target_link_libraries(tests PRIVATE Catch2::Catch2 GDAL::GDAL ${PNG_LIBRARY})
target_compile_features(tests PRIVATE cxx_std_17)
