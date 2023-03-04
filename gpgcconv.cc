#include "gpgc.hpp"
#include <cstdlib>

//#define __GPGC_DEBUG_VERBOSE

#define STR_ENDS_WITH(S, E) (strcmp(S + strlen(S) - (sizeof(E)-1), E) == 0)

int tif2gpgc(char* inname, char* outname, float zeta, int mu, bool max_error) {
	if(STR_ENDS_WITH(inname, ".tif")) {
		gpgc_gdal_data dat = process_file(inname);
		
		gpgc_encode(inname, outname, dat, zeta, mu, max_error);
		std::cout << outname << "\t" << outname << ".log\n";
	}
	std::cout << "\n\n";
	return 0;

}

int gpgc2png(char* inname, char* outname) {
    gpgc_header_t decoded_head;
    gpgc_vector* dcmp_nodes = gpgc_read(inname, &decoded_head);

    std::vector<float> x0, y0;
    int tmp = gpgc_decode_offsets(dcmp_nodes, decoded_head, x0, y0);

    int** raster = gpgc_reconstruct(dcmp_nodes, decoded_head, x0, y0);
#ifdef __GPGC_DEBUG_VERBOSE
    for(int i = 0; i < decoded_head.height; ++i) {
        for(int j = 0; j < decoded_head.width; ++j)
            std::cout << raster[i][j] << " ";
        std::cout << "\n";
    }
#endif
    save_png(outname, raster, decoded_head.width, decoded_head.height);
	std::cout << outname << std::endl;
	delete dcmp_nodes;
	delete[] raster;
    return 0;
}

int gpgc2benchmark(char* inname) {
    gpgc_header_t decoded_head;
    gpgc_vector* dcmp_nodes = gpgc_read(inname, &decoded_head);

    std::vector<float> x0, y0;
    int tmp = gpgc_decode_offsets(dcmp_nodes, decoded_head, x0, y0);

    int** raster = gpgc_reconstruct(dcmp_nodes, decoded_head, x0, y0);
	delete dcmp_nodes;
	delete[] raster;
    return 0;
}

int put_h() {
	puts("Usage: gpgcconv <infile> <outfile>");
	puts("Examples:");
	puts("	gpgcconv input.tif output.gpgc [-u Partition Standard Deviation (int)] [-z Partition max entropy (double)");
	puts("	gpgcconv input.gpgc output.png");
	return 0;
}

int main(int argc, char **argv) {
	if(argc < 3)
        put_h();

	int mu = 5;
	double zeta = 0.3;
	bool is_verbose = false, max_error = true;

	std::streambuf* cout_buffer = std::cout.rdbuf();	

	for(int i = 0; i < argc; i++) {
        std::string fl = argv[i];
		if(fl == "-z" || fl == "--zeta")
			i + 1 <= argc ? zeta = atof(argv[i + 1]) : put_h();
		if(fl == "-u" || fl == "--mu")
			i + 1 <= argc ? mu = atoi(argv[i + 1]) : put_h();
		if(fl == "--verbose") is_verbose = true;
		if(fl == "--no-max-error") max_error = false;
	
	}

	if(is_verbose) std::cout.rdbuf(NULL);

	if(STR_ENDS_WITH(argv[1], ".gpgc") && strcmp(argv[2], "GPGC_BENCHMARK"))
		gpgc2benchmark(argv[1]);
	
	if(STR_ENDS_WITH(argv[1], ".tif") && STR_ENDS_WITH(argv[2], ".gpgc"))
		tif2gpgc(argv[1], argv[2], zeta, mu, max_error);

	if(STR_ENDS_WITH(argv[1], ".gpgc") && STR_ENDS_WITH(argv[2], ".png"))
		gpgc2png(argv[1], argv[2]);


	return 0;
}
