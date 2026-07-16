#include <gtest/gtest.h>

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <iterator>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "wfrest/ErrorCode.h"
#include "wfrest/FileUtil.h"
#include "FileTestUtil.h"

using namespace wfrest;

namespace
{

std::string test_root(const std::string &name)
{
    return "/tmp/wfrest-fileutil-" + name + "-" +
           std::to_string(getpid());
}

bool path_exists(const std::string &path)
{
    struct stat metadata;
    return lstat(path.c_str(), &metadata) == 0;
}

bool is_directory(const std::string &path)
{
    struct stat metadata;
    return stat(path.c_str(), &metadata) == 0 &&
           S_ISDIR(metadata.st_mode);
}

off_t file_size(const std::string &path)
{
    struct stat metadata;
    return stat(path.c_str(), &metadata) == 0 ? metadata.st_size : -1;
}

std::string read_file(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

} // namespace

TEST(FileUtil, size_classifies_results_without_invalid_access)
{
    const std::string root = test_root("size");
    ASSERT_TRUE(FileUtil::create_directories(root));
    const std::string file = root + "/regular.txt";
    ASSERT_TRUE(FileTestUtil::write_file(file, "Writing this to a file.\n"));

    size_t result = 99;
    EXPECT_TRUE(FileUtil::file_exists(file));
    EXPECT_FALSE(FileUtil::file_exists(root));
    EXPECT_EQ(FileUtil::size(file, &result), StatusOK);
    EXPECT_EQ(result, 24U);

    result = 99;
    EXPECT_EQ(FileUtil::size(root + "/missing", &result), StatusNotFound);
    EXPECT_EQ(result, 0U);

    result = 99;
    EXPECT_EQ(FileUtil::size(root, &result), StatusFileReadError);
    EXPECT_EQ(result, 0U);
    EXPECT_EQ(FileUtil::size(file, nullptr), StatusFileReadError);

    const std::string link = root + "/regular-link";
    ASSERT_EQ(symlink(file.c_str(), link.c_str()), 0);
    result = 0;
    EXPECT_EQ(FileUtil::size(link, &result), StatusOK);
    EXPECT_EQ(result, 24U);

    EXPECT_TRUE(FileUtil::remove_directory(root));
    EXPECT_FALSE(path_exists(root));
}

TEST(FileUtil, create_directories_checks_every_component)
{
    const std::string root = test_root("mkdir");
    const std::string nested = root + "//a/b///c/";
    const std::string relative_root =
        "./wfrest-fileutil-relative-" + std::to_string(getpid());
    const std::string relative_nested = relative_root + "/a/b/";
    EXPECT_FALSE(FileUtil::create_directories(""));
    EXPECT_TRUE(FileUtil::create_directories("/"));
    ASSERT_TRUE(FileUtil::create_directories(nested));
    ASSERT_TRUE(FileUtil::create_directories(relative_nested));
    EXPECT_TRUE(is_directory(root + "/a/b/c"));
    EXPECT_TRUE(is_directory(relative_nested));
    EXPECT_TRUE(FileUtil::create_directories(nested));

    const std::string blocker = root + "/blocker";
    ASSERT_TRUE(FileTestUtil::write_file(blocker, "not a directory"));
    EXPECT_FALSE(FileUtil::create_directories(blocker));
    EXPECT_FALSE(FileUtil::create_directories(blocker + "/"));
    EXPECT_FALSE(FileUtil::create_directories(blocker + "/child"));

    EXPECT_TRUE(FileUtil::remove_directory(root));
    EXPECT_TRUE(FileUtil::remove_directory(relative_root));
    EXPECT_FALSE(path_exists(root));
    EXPECT_FALSE(path_exists(relative_root));
}

TEST(FileUtil, remove_directory_never_follows_symlinks)
{
    const std::string root = test_root("remove");
    const std::string target = root + "/target";
    const std::string nested = target + "/a/b";
    const std::string outside = root + "/outside";
    ASSERT_TRUE(FileUtil::create_directories(nested));
    ASSERT_TRUE(FileUtil::create_directories(outside));
    ASSERT_TRUE(FileTestUtil::write_file(nested + "/inside.txt", "inside"));
    const std::string outside_file = outside + "/keep.txt";
    ASSERT_TRUE(FileTestUtil::write_file(outside_file, "keep"));

    const std::string child_link = target + "/escape";
    ASSERT_EQ(symlink(outside.c_str(), child_link.c_str()), 0);
    const std::string root_link = root + "/outside-link";
    ASSERT_EQ(symlink(outside.c_str(), root_link.c_str()), 0);

    EXPECT_FALSE(FileUtil::remove_directory(root_link));
    EXPECT_FALSE(FileUtil::remove_directory(root_link + "/."));
    EXPECT_TRUE(path_exists(outside_file));
    EXPECT_EQ(read_file(outside_file), "keep");

    EXPECT_TRUE(FileUtil::remove_directory(target + "/./"));
    EXPECT_FALSE(path_exists(target));
    EXPECT_TRUE(path_exists(outside_file));
    EXPECT_EQ(read_file(outside_file), "keep");
    EXPECT_FALSE(FileUtil::remove_directory(outside_file));
    EXPECT_FALSE(FileUtil::remove_directory(root + "/missing"));

    unlink(root_link.c_str());
    EXPECT_TRUE(FileUtil::remove_directory(root));
    EXPECT_FALSE(path_exists(root));
}

TEST(FileUtil, remove_directory_rejects_current_directory)
{
    const std::string root = test_root("current");
    ASSERT_TRUE(FileUtil::create_directories(root));
    ASSERT_TRUE(FileTestUtil::write_file(root + "/sentinel", "keep"));

    const int original_cwd = open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    ASSERT_GE(original_cwd, 0);
    ASSERT_EQ(chdir(root.c_str()), 0);
    EXPECT_FALSE(FileUtil::remove_directory("."));
    EXPECT_TRUE(path_exists("./sentinel"));
    ASSERT_EQ(fchdir(original_cwd), 0);
    ASSERT_EQ(close(original_cwd), 0);

    EXPECT_TRUE(FileUtil::remove_directory(root));
}

TEST(FileUtil, create_file_with_size_reports_complete_writes)
{
    const std::string root = test_root("generate");
    ASSERT_TRUE(FileUtil::create_directories(root));
    const std::string path = root + "/generated.bin";

    ASSERT_TRUE(FileUtil::create_file_with_size(path, 9000));
    EXPECT_EQ(file_size(path), 9000);
    const std::string content = read_file(path);
    ASSERT_EQ(content.size(), 9000U);
    EXPECT_TRUE(std::all_of(content.begin(), content.end(), [](char ch)
    {
        const unsigned char byte = static_cast<unsigned char>(ch);
        return byte >= 32 && byte <= 126;
    }));

    EXPECT_TRUE(FileUtil::create_file_with_size(path, 0));
    EXPECT_EQ(file_size(path), 0);
    EXPECT_FALSE(FileUtil::create_file_with_size(
        root + "/missing/child.bin", 1));
    EXPECT_FALSE(FileUtil::create_file_with_size(root, 1));
    if (access("/dev/full", W_OK) == 0)
    {
        EXPECT_FALSE(FileUtil::create_file_with_size("/dev/full", 4096));
    }

    EXPECT_TRUE(FileUtil::remove_directory(root));
}
