#include "wfrest/ErrorCode.h"
#include <gtest/gtest.h>
#include "wfrest/Compress.h"

using namespace wfrest;

TEST(gzip, shortText)
{
    std::string str = "WFREST compress : Just for test....";
    std::string compress_str;
    int ret = Compressor::gzip(&str, &compress_str);
    EXPECT_EQ(ret, StatusOK);
    std::string decompress_str;
    ret = Compressor::ungzip(&compress_str, &decompress_str);
    EXPECT_EQ(ret, StatusOK);
    EXPECT_EQ(str, decompress_str);
}

TEST(gzip, longText)
{
    std::string str;
    for (size_t i = 0; i < 100000; i++)
    {
        str.append(std::to_string(i));
    }
    auto *compress_str = new std::string;
    int ret = Compressor::gzip(&str, compress_str);
    EXPECT_EQ(ret, StatusOK);

    auto *decompress_str = new std::string;
    ret = Compressor::ungzip(compress_str, decompress_str);
    EXPECT_EQ(ret, StatusOK);
    EXPECT_EQ(str, *decompress_str);
    delete compress_str;
    delete decompress_str;
}
