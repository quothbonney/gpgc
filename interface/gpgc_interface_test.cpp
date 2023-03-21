//
// Created by quothbonney on 3/6/23.
//

#include "../gpgc.hpp"
#include "gpgc_python_interface.hpp"

int main() {
    auto dat = process_file("../../data/test1.tif");
    const char* fname = "../../data/test1.tif";
    auto v = gpgc_encode_bytes(const_cast<char*>(fname), dat, 0.5, 5, true);

    return 0;
}