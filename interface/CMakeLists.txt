cmake_minimum_required(VERSION 3.4)
project(gpgc_pybind)

find_package(Python3 COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG)

pybind11_add_module(example gpgc_python_interface.cpp)
