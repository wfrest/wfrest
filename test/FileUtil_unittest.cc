#include <gtest/gtest.h>
#include <fstream>
#include "wfrest/FileUtil.h"
#include "FileTestUtil.h"

using namespace wfrest;

TEST(FileUtil, size)
{
    std::string file_path = "./example.txt";
    // 1. Write a sample text first
    bool write_ok = FileTestUtil::write_file(file_path, "Writing this to a file.\n");
    EXPECT_EQ(write_ok, true);

    // 2. Then we get the file size
    size_t sample_size;
    int ret = FileUtil::size(file_path, &sample_size);
    EXPECT_TRUE(ret == 0);
    EXPECT_EQ(sample_size, 24);

    // 3. At last we delete this tmp file
    std::remove(file_path.c_str());
    EXPECT_FALSE(FileUtil::file_exists(file_path));
}

TEST(FileUtil, file_exists)
{
    std::string file_path = "./example.md";
    bool write_ok = FileTestUtil::write_file(file_path, "Writing this to a file.\n");
    EXPECT_EQ(write_ok, true);

    EXPECT_TRUE(FileUtil::file_exists(file_path));

    std::remove(file_path.c_str());
    EXPECT_FALSE(FileUtil::file_exists(file_path));
}

TEST(FileUtil, file_exists_txt)
{
    std::string file_path = "./example.txt";
    bool write_ok = FileTestUtil::write_file(file_path, "Writing this to a file.\n");
    EXPECT_EQ(write_ok, true);

    EXPECT_TRUE(FileUtil::file_exists(file_path));

    std::remove(file_path.c_str());
    EXPECT_FALSE(FileUtil::file_exists(file_path));
}



