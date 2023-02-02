#include "gpgc.hpp"
#include "half/half/half.hpp"
#include <cstdint>
#include <sys/types.h>

void gpgc_read(const char* filename, const int size) {
    std::ifstream gpgc_file(filename, std::ios::binary);

	gpgc_header_t header{};

	uint64_t x;
	
	uint32_t x_header;
	gpgc_file.read(reinterpret_cast<char*>(&x_header), sizeof(uint32_t));
	if(x_header != GPGC_MAGIC_DECIMAL) {
		header.magic = x_header;
		exit(1);	
	}
	gpgc_file.read(reinterpret_cast<char*>(&x_header), sizeof(uint32_t));
	header.width = x_header;
	gpgc_file.read(reinterpret_cast<char*>(&x_header), sizeof(uint32_t));
	header.height = x_header;
	gpgc_file.read(reinterpret_cast<char*>(&x_header), sizeof(uint32_t));
	header.node_count = x_header;

	gpgc_vector* decomp_nodes = new gpgc_vector[header.node_count];

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
    }


    gpgc_file.close();
}
