#include <gtest/gtest.h>
#include <fstream>
#include "wfrest/FileUtil.h"

using namespace wfrest;

TEST(FileUtil, size)
{
    // 1. Write a sample text first

    std::ofstream sample_file;
    sample_file.open("example.txt", std::ios::out | std::ios::trunc);
    sample_file << "Writing this to a file.\n";
    sample_file.close();

    // 2. Then we get the file size
    size_t sample_size;
    int ret = FileUtil::size("./example.txt", &sample_size);
    EXPECT_TRUE(ret == 0);
    EXPECT_EQ(sample_size, 24);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


