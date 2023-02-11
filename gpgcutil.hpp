
#ifndef GPGC_GPGCUTIL_H
#define GPGC_GPGCUTIL_H

#define CM_DEFAULT std::vector<std::string>{"#1C0B4B", "#237672", "#FFF700"}

#include <iostream>

int print_progress(float progress);

std::vector<unsigned char> gradient_map(int num_colors, const std::vector<std::string>& hex_codes);
#endif //GPGC_GPGCUTIL_H
