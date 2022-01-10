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
    std::string dir_path = "./a/b/tmp/test_dir";
    int ret = FileTestUtil::mkpath(dir_path.c_str(), 0777);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(PathUtil::is_dir(dir_path));

    // Delete dir
    FileTestUtil::recursive_delete(dir_path.c_str());
    EXPECT_FALSE(PathUtil::is_dir(dir_path));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}