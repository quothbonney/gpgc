#include "gpgc.hpp"

int print_progress(float progress) {
	int barWidth = 70;

	std::cout << "[";
	int pos = barWidth * progress;
	for (int i = 0; i < barWidth; ++i) {
		if (i < pos) std::cout << "=";
		else if (i == pos) std::cout << ">";
		else std::cout << " ";
	}
	std::cout << "] " << int(progress * 100.0) << " %\r";
	std::cout.flush();
	
	return 0;
}

std::vector<unsigned char> gradient_map(int num_colors, const std::vector<std::string>& hex_codes) {
    std::vector<unsigned char> color_map(num_colors * 3);
    int num_hex = hex_codes.size();

    // calculate the gradient between each pair of adjacent hex codes
    for (int i = 0; i < num_colors; i++) {
        int color_index = i * 3;
        int hex_index = (int)((double)i * (num_hex - 1) / (num_colors - 1));
        int r1, g1, b1, r2, g2, b2;
        sscanf(hex_codes[hex_index].c_str(), "#%02x%02x%02x", &r1, &g1, &b1);
        sscanf(hex_codes[std::min(hex_index + 1, num_hex - 1)].c_str(), "#%02x%02x%02x", &r2, &g2, &b2);
        color_map[color_index + 0] = (unsigned char)(r1 + (r2 - r1) * (i - hex_index * (num_colors - 1) / (num_hex - 1)) / (num_colors - 1));
        color_map[color_index + 1] = (unsigned char)(g1 + (g2 - g1) * (i - hex_index * (num_colors - 1) / (num_hex - 1)) / (num_colors - 1));
        color_map[color_index + 2] = (unsigned char)(b1 + (b2 - b1) * (i - hex_index * (num_colors - 1) / (num_hex - 1)) / (num_colors - 1));
    }
    return color_map;
}
