#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <iostream>
#include <filesystem>
#include "../gpgc.hpp"

TEST_CASE("GPGC Encode Binary Completes") {
    std::vector<const char*> test_files{
            "../data/test1.tif",
            "../data/test2.tif",
            "../data/768.tif"
    };

    std::ofstream nullstream("data/null.txt");
    std::streambuf * old_cout = std::cout.rdbuf();
    std::cout.rdbuf(nullstream.rdbuf());
    for (const auto& testfile : test_files) {
        gpgc_gdal_data dat = process_file(testfile);
        gpgc_encode(const_cast<char *>(testfile), "./test_output.gpgc", dat, 0.2, 5, true);
        std::uintmax_t filesize1 = std::filesystem::file_size("./test_output.gpgc");
        CHECK(filesize1 > 200);


        gpgc_header_t decoded_head;
        gpgc_vector* dcmp_nodes = gpgc_read("./test_output.gpgc", &decoded_head);

        std::vector<float> x0, y0;
        int tmp = gpgc_decode_offsets(dcmp_nodes, decoded_head, x0, y0);

        int** raster = gpgc_reconstruct(dcmp_nodes, decoded_head, x0, y0);

        delete dcmp_nodes;
        delete[] raster;
        std::cout.rdbuf(old_cout);
        CHECK(filesize1 > 200);
        std::cout.rdbuf(nullstream.rdbuf());
    }
    std::filesystem::remove("./test_output.gpgc");
}
