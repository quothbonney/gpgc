#include "gpgc.hpp"
#include "half/half/half.hpp"
#include <cstdint>
#include <sys/types.h>

gpgc_vector* gpgc_read(const char* filename, const int size, gpgc_header_t* head) {
    std::ifstream gpgc_file(filename, std::ios::binary);

	gpgc_header_t header{};

	uint64_t x;
	
	uint32_t x_header;
	gpgc_file.read(reinterpret_cast<char*>(&x_header), sizeof(uint32_t));
	if(x_header != GPGC_MAGIC_DECIMAL) {
		header.magic = x_header;
		std::cout << x_header;
		exit(1);	
	}
	gpgc_file.read(reinterpret_cast<char*>(&x_header), sizeof(uint32_t));
	header.width = x_header;
	gpgc_file.read(reinterpret_cast<char*>(&x_header), sizeof(uint32_t));
	header.height = x_header;
	gpgc_file.read(reinterpret_cast<char*>(&x_header), sizeof(uint32_t));
	header.node_count = x_header;

	*head = header;

	gpgc_vector* decomp_nodes = new gpgc_vector[header.node_count];

    gpgc_file.seekg(2*GPGC_HEADER_SIZE, std::ios::beg);
	size_t index = 0;
    while (gpgc_file.read(reinterpret_cast<char*>(&x), sizeof(uint64_t))) {
		half_float::half i, j;
		int16_t k;
		u_int16_t p_sz;
		uint64_t* bblock = new uint64_t[4];
		memcpy(bblock, &x, sizeof(struct gpgc_vector));
    
		p_sz = (u_int16_t) ((0xFFFF000000000000 & bblock[0]) >> 48);
		k = (int16_t) ((0x0000FFFF00000000 & bblock[0]) >> 32);
		// Half precision float library wont allow direct recast to float
		// Memory must be manually bitshifted. This puts the representation of
		// the half float into a 16 bit integer and then forces it into a half-
		// float container
		auto j_int = (int16_t) ((0x00000000FFFF0000 & bblock[0]) >> 16);
		auto i_int = (int16_t) ((0x000000000000FFFF & bblock[0]) >> 0);

		memcpy(&i, &i_int, sizeof(i));
		memcpy(&j, &j_int, sizeof(i));

		decomp_nodes[index] = gpgc_vector{i, j, k, p_sz};
		delete[] bblock;
        index++;
    }

    gpgc_file.close();

	return decomp_nodes;
}

int gpgc_decode_offsets(gpgc_vector* dc_vectors, const gpgc_header_t& header, std::vector<float>& x0, std::vector<float>& y0) {
	if(x0.size() != 0 || y0.size() != 0)
		std::cerr << "Corrupted x0 or y0 vector";

    // Start each vector out with 0 so it will go from 0 to 1
	x0.push_back(0);
	y0.push_back(0);
    // b is buffer size vector
    std::vector<int> b = { 0 };
    int num_vectors = header.node_count;
    // Allocate memory for each vector to allow insertions without segfaults probably
    x0.reserve(num_vectors*2);
    y0.reserve(num_vectors*2);
    b.reserve(num_vectors * 4);

    std::vector<int> logsizes;
    logsizes.reserve(num_vectors*2);

    // Define iterators for index positions
    const auto it_b = b.cbegin();
    const auto it_y = y0.begin();
    const auto it_x = x0.cbegin();
    size_t index = 0;

    for(int i = 0; i < num_vectors; ++i) {
        if(dc_vectors[i].size == 0) {
            num_vectors = i;
            break;
        }
        int logsize = std::log2(header.height / dc_vectors[i].size);
        logsizes.push_back(logsize);
    }
    while (index < num_vectors) {
        if(index % 10 == 0) {
            index = index;
        }
        while (b[index] < logsizes[index]) {
            b[index] = b[index] + 1;
            for (int i = 0; i < 3; ++i)
                b.insert(it_b + index + 1, b[index]);

            x0.insert(it_x + index + 1, x0[index] + (1 / pow(2, b[index])));
            x0.insert(it_x + index + 2, x0[index]);
            x0.insert(it_x + index + 3, x0[index] + (1 / pow(2, b[index])));

            y0.insert(it_y + index + 1, y0[index]);
            y0.insert(it_y + index + 2, y0[index] + (1 / pow(2, b[index])));
            y0.insert(it_y + index + 3, y0[index] + (1 / pow(2, b[index])));
        }
        index++;
    }
    return 0;
}


int** gpgc_reconstruct(gpgc_vector* dc_vectors, const gpgc_header_t& header, std::vector<float>& x0, std::vector<float>& y0) {
    int** reras = new int*[header.height];
    for(int i = 0; i < header.height; ++i) {
        reras[i] = new int[header.width];
    }

    for(int index = 0; index < header.node_count; index++) {
        int xoff = x0[index] * header.width;
        int yoff = y0[index] * header.height;

        int sz = dc_vectors[index].size;
        for(int y = 0; y < sz; ++y){
            for(int x = 0; x < sz; ++x) {
                int val = (dc_vectors[index].i*y) + (dc_vectors[index].j*x) + (dc_vectors[index].k);
                reras[yoff+y][xoff+x] = val;
            }
        }
    }
    return reras;
}

bool save_png(const char* filename, int** image, int width, int height) {
    FILE* fp = std::fopen(filename, "wb");
    if (!fp) {
        return false;
    }

    // Get largest value in the raster and to figure out how much each point needs to be divided by to get a value <255 to fit into PNG uint8
    uint16_t max_int = 0;
    for(int i = 0; i < height; ++i) {
        for(int j = 0; j < width; ++j)
            image[i][j] > max_int ? max_int = image[i][j] : max_int = max_int;
    }
    float shrink_coeff = (float) max_int/ 256;
    auto condense_for_png = [shrink_coeff](int x) {
        return (int) (x / shrink_coeff);
    };
    // Initialize the PNG write struct
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    png_init_io(png_ptr, fp);

    // Set the image parameters
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);
    int** rgb_image = new int*[height];
    png_byte *row = new png_byte[width * 3];

    std::vector<unsigned char> color_map = gradient_map(256);
    for (int i = 0; i < height; i++) {
        rgb_image[i] = new int[width * 3];
        for (int j = 0; j < width; j++) {
            uint8_t value = condense_for_png(image[i][j]);
            row[j * 3 + 0] = color_map[value * 3 + 0];     // red channel
            row[j * 3 + 1] = color_map[value * 3 + 1];;    // green channel
            row[j * 3 + 2] = color_map[value * 3 + 2];;    // blue channel
        }
        png_write_row(png_ptr, row);
    }



    // Allocate memory for the row pointers
    png_bytep* row_pointers = new png_bytep[height];
    for (int i = 0; i < height; i++) {
        row_pointers[i] = reinterpret_cast<png_byte*>(rgb_image[i]);
    }

    // Write the image data
    //png_write_info(png_ptr, info_ptr);
    //png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, nullptr);

    // Clean up
    delete[] row_pointers;
    for (int i = 0; i < height; i++) {
        delete[] image[i];
    }
    delete[] image;
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return 0;
}