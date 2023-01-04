#include "gpgc.hpp"
#include <cstdlib>

#define STR_ENDS_WITH(S, E) (strcmp(S + strlen(S) - (sizeof(E)-1), E) == 0)

int put_h() {
	puts("Usage: gpgcconv <infile> <outfile>");
	puts("Examples:");
	puts("	gpgcconv input.tif output.gpgc [-u Partition Standard Deviation (int)] [-z Partition max entropy (double)");
	puts("	gpgcconv input.gpgc output.png");
	return 0;
}

int main(int argc, char **argv) {
	if(argc < 3) {
		puts("Usage: gpgcconv <infile> <outfile>");
		puts("Examples:");
		puts("	gpgcconv input.tif output.gpgc [-u Partition Standard Deviation (int)] [-z Partition max entropy (double)");
		puts("	gpgcconv input.gpgc output.png");
		exit(1);
	}

	int mu = 20;
	double zeta = 0.3;
	bool is_verbose = false;

	std::streambuf* cout_buffer = std::cout.rdbuf();	

	for(int i = 0; i < argc; i++) {
        std::string fl = argv[i];
		if(fl == "-z" || fl == "--zeta")
			i + 1 <= argc ? zeta = atof(argv[i + 1]) : put_h();
		if(fl == "-u" || fl == "--mu")
			i + 1 <= argc ? mu = atoi(argv[i + 1]) : put_h();
		if(fl == "--verbose") is_verbose = true;
	}
	if(is_verbose) std::cout.rdbuf(NULL);
	
	if(STR_ENDS_WITH(argv[1], ".tif")) {
		gpgc_gdal_data dat = process_file(argv[1]);

		
		if(is_verbose) std::cout.rdbuf(cout_buffer);
		gpgc_encode(argv[1], argv[2], dat, zeta, mu);
	}
	std::cout.rdbuf(cout_buffer);
	std::cout << argv[2] << "\t" << argv[2] << ".log\n";

	return 0;
}
