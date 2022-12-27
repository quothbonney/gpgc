#include "gpgc.hpp"
#include "stb_image.h"
#include "stb_image_write.h"

#define STR_ENDS_WITH(S, E) (strcmp(S + strlen(S) - (sizeof(E)-1), E) == 0)

int main(int argc, char **argv) {
	if(argc < 3) {
		puts("Usage: gpgcconv <infile> <outfile>");
		puts("Examples:");
		puts("	gpgcconv input.tif output.gpgc");
		puts("	gpgcconv input.gpgc output.png");
		exit(1);
	}
	
	if(STR_ENDS_WITH(argv[1], ".tif")) {
		gpgc_gdal_data dat = process_file(argv[1]);
		gpgc_encode(argv[2], dat);
	}

	return 0;
}
