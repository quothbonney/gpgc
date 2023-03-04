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
    }
    std::cout.rdbuf(old_cout);
    std::filesystem::remove("./test_output.gpgc");

}