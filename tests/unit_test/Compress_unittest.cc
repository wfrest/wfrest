#include <gtest/gtest.h>
#include "Compress.h"

using namespace wfrest;

TEST(Compressor, gzip)
{
    std::string str = "WFREST compress : Just for test....";
    std::string compress_str = Compressor::gzip(str.c_str(), str.size());
    std::string decompress_str = Compressor::ungzip(compress_str.c_str(), compress_str.size());

    EXPECT_EQ(str, decompress_str);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}