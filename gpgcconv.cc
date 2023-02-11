#include "gpgc.hpp"
#include <cstdlib>
#include <thread>

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

    for(int i = 0; i < decoded_head.node_count; ++i) {
        std::cout << x0[i] << "\t" << y0[i] << "\n";
    }
    int** raster = gpgc_reconstruct(dcmp_nodes, decoded_head, x0, y0);

    save_png(outname, raster, decoded_head.width, decoded_head.height);
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
	
	if(STR_ENDS_WITH(argv[1], ".tif") && STR_ENDS_WITH(argv[2], ".gpgc"))
		tif2gpgc(argv[1], argv[2], zeta, mu, max_error);

	if(STR_ENDS_WITH(argv[1], ".gpgc") && STR_ENDS_WITH(argv[2], ".png"))
		gpgc2png(argv[1], argv[2]);


	return 0;
}
