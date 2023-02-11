
#ifndef GPGC_GPGCUTIL_H
#define GPGC_GPGCUTIL_H

#define CM_DEFAULT std::vector<std::string>{"#1C0B4B", "#237672", "#FFF700"}

#include <iostream>
#include <vector>

// Cute progress bar utility for encoding
int print_progress(float progress);

// Creates 255 bit gradient map for PNG-ization using std::vector of hex values as strings
// Returns the 0-255 color value passed to the `gpgcdecode.cc` save_png() function
std::vector<unsigned char> gradient_map(int num_colors, const std::vector<std::string>& hex_codes);

#endif //GPGC_GPGCUTIL_H
