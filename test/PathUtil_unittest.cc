#include <unordered_map>
#include <gtest/gtest.h>
#include "wfrest/PathUtil.h"
#include "FileTestUtil.h"

using namespace wfrest;

TEST(PathUtil, is_dir)
{
    std::string dir_path = "test_dir";
    int ret = FileTestUtil::create_dir(dir_path.c_str(), 0777);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(PathUtil::is_dir(dir_path));

    // Delete dir
    FileTestUtil::recursive_delete(dir_path.c_str());
    EXPECT_FALSE(PathUtil::is_dir(dir_path));
}

TEST(PathUtil, path_is_dir)
{
    std::string root_dir = "./a";
    std::string dir_path = root_dir + "/b/tmp/test_dir";
    int ret = FileTestUtil::mkpath(dir_path.c_str(), 0777);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(PathUtil::is_dir(dir_path));

    // Delete dir
    FileTestUtil::recursive_delete(root_dir.c_str());
    EXPECT_FALSE(PathUtil::is_dir(dir_path));
}

TEST(PathUtil, is_file)
{
    std::string file_path = "./test.txt";
    bool write_ok = FileTestUtil::write_file(file_path, "test text\n");
    EXPECT_EQ(write_ok, true);
    EXPECT_TRUE(PathUtil::is_file(file_path));

    std::remove(file_path.c_str());
    EXPECT_FALSE(PathUtil::is_file(file_path));
}

TEST(PathUtil, path_is_file)
{
    std::string root_dir = "./a";
    std::string dir_path = root_dir + "/b/tmp/test_dir";
    int ret = FileTestUtil::mkpath(dir_path.c_str(), 0777);
    EXPECT_EQ(ret, 0);

    std::string file_path = dir_path + "/test.txt";
    bool write_ok = FileTestUtil::write_file(file_path, "test text\n");
    EXPECT_EQ(write_ok, true);
    EXPECT_TRUE(PathUtil::is_file(file_path));

    std::remove(file_path.c_str());
    EXPECT_FALSE(PathUtil::is_file(file_path));
    FileTestUtil::recursive_delete(root_dir.c_str());
    EXPECT_FALSE(PathUtil::is_dir(dir_path));
}

TEST(PathUtil, concat_path)
{
    std::string concat1 = PathUtil::concat_path("/v1/v2", "v3");
    EXPECT_EQ(concat1, "/v1/v2/v3");

    std::string concat2 = PathUtil::concat_path("/v1/v2/", "v3");
    EXPECT_EQ(concat2, "/v1/v2/v3");

    std::string concat3 = PathUtil::concat_path("/v1/v2/", "/v3");
    EXPECT_EQ(concat3, "/v1/v2/v3");

    std::string concat4 = PathUtil::concat_path("/v1/v2", "/v3");
    EXPECT_EQ(concat4, "/v1/v2/v3");
}

TEST(PathUtil, base)
{
    EXPECT_EQ(PathUtil::base("/usr/local/image/test.jpg"), "test.jpg");
    EXPECT_EQ(PathUtil::base("/usr/local/demo.xml"), "demo.xml");
    EXPECT_EQ(PathUtil::base("/usr/local/demo"), "demo");
}

TEST(PathUtil, suffix)
{
    EXPECT_EQ(PathUtil::suffix("/usr/local/image/test.jpg"), "jpg");
    EXPECT_EQ(PathUtil::suffix("/usr/local/demo.xml"), "xml");
    EXPECT_EQ(PathUtil::suffix("/usr/local/demo"), "");
}
