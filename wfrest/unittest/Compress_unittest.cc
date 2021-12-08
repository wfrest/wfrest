#include <gtest/gtest.h>
#include "wfrest/Compress.h"

using namespace wfrest;

TEST(gzip, shortText)
{
    std::string str = "WFREST compress : Just for test....";
    std::string compress_str = Compressor::gzip(str.c_str(), str.size());
    EXPECT_TRUE(compress_str.empty() == false);
    std::string decompress_str = Compressor::ungzip(compress_str.c_str(), compress_str.size());
    EXPECT_EQ(str, decompress_str);
}

TEST(gzip, longText)
{
    std::string str;
    for (size_t i = 0; i < 100000; i++)
    {
        str.append(std::to_string(i));
    }
    auto compress_str = Compressor::gzip(str.c_str(), str.size());
    EXPECT_TRUE(compress_str.empty() == false);
    auto decompress_str =
        Compressor::ungzip(compress_str.c_str(), compress_str.size());
    EXPECT_EQ(str, decompress_str);
}

TEST(brotli, shortText)
{
    std::string str = "WFREST compress : Just for test....";
    std::string compress_str = Compressor::brotli(str.c_str(), str.size());
    EXPECT_TRUE(compress_str.empty() == false);
    std::string decompress_str 
            = Compressor::unbrotli(compress_str.c_str(), compress_str.size());
    EXPECT_EQ(str, decompress_str);
}

TEST(brotli, longText)
{
    std::string str;
    for (size_t i = 0; i < 100000; i++)
    {
        str.append(std::to_string(i));
    }
    auto compress_str = Compressor::brotli(str.c_str(), str.size());
    EXPECT_TRUE(compress_str.empty() == false);
    auto decompress_str 
            = Compressor::unbrotli(compress_str.c_str(), compress_str.size());
    EXPECT_EQ(str, decompress_str);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}