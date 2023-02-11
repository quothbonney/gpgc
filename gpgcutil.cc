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

std::vector<unsigned char> gradient_map(int num_colors) {
    std::vector<unsigned char> color_map(num_colors * 3);
    for (int i = 0; i < num_colors; i++) {
        color_map[i * 3 + 0] = (unsigned char)(255 * i / (num_colors - 1));
        color_map[i * 3 + 1] = 0;
        color_map[i * 3 + 2] = (unsigned char)(255 * (num_colors - i - 1) / (num_colors - 1));
    }
    return color_map;
}