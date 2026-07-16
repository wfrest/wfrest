#include <gtest/gtest.h>

#include <atomic>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#include "wfrest/FileCache.h"

using namespace wfrest;

class FileCacheTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        cache_.enable();
        cache_.clear();
        cache_.set_max_size(kDefaultMaxSize);
    }

    void TearDown() override
    {
        cache_.enable();
        cache_.clear();
        cache_.set_max_size(kDefaultMaxSize);
        for (const auto& path : paths_)
            std::remove(path.c_str());
    }

    std::string create_file(const std::string& content)
    {
        std::string path = "/tmp/wfrest-file-cache-" +
                           std::to_string(static_cast<long long>(getpid())) + "-" +
                           std::to_string(paths_.size());
        paths_.push_back(path);

        std::ofstream output(path.c_str(), std::ios::binary | std::ios::trunc);
        EXPECT_TRUE(output.is_open());
        output.write(content.data(), content.size());
        EXPECT_TRUE(output.good());
        return path;
    }

    void overwrite_file(const std::string& path, const std::string& content)
    {
        std::ofstream output(path.c_str(), std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(output.is_open());
        output.write(content.data(), content.size());
        ASSERT_TRUE(output.good());
    }

    std::time_t modification_time(const std::string& path)
    {
        struct stat file_stat;
        EXPECT_EQ(stat(path.c_str(), &file_stat), 0);
        return file_stat.st_mtime;
    }

    static const size_t kDefaultMaxSize = 100 * 1024 * 1024;
    FileCache& cache_ = FileCache::instance();
    std::vector<std::string> paths_;
};

const size_t FileCacheTest::kDefaultMaxSize;

TEST_F(FileCacheTest, range_reads_are_half_open_and_clamped)
{
    const std::string content = "abcdef";
    const std::string path = create_file(content);
    cache_.add_file(path, content, modification_time(path));

    std::string output;
    EXPECT_TRUE(cache_.get_file(path, output));
    EXPECT_EQ(output, "abcdef");

    EXPECT_TRUE(cache_.get_file(path, output, 1, 4));
    EXPECT_EQ(output, "bcd");

    EXPECT_TRUE(cache_.get_file(path, output, 4, 99));
    EXPECT_EQ(output, "ef");

    EXPECT_TRUE(cache_.get_file(path, output, 99, 4));
    EXPECT_TRUE(output.empty());
}

TEST_F(FileCacheTest, replacement_has_exact_byte_accounting)
{
    cache_.add_file("replacement", std::string(8, 'a'), 1);
    EXPECT_EQ(cache_.size(), 8U);

    cache_.add_file("replacement", std::string(3, 'b'), 2);
    EXPECT_EQ(cache_.size(), 3U);
}

TEST_F(FileCacheTest, insertion_reserves_capacity_and_rejects_oversized_entries)
{
    cache_.set_max_size(100);
    for (size_t i = 0; i < 70; ++i)
        cache_.add_file("small-" + std::to_string(i), "x", 1);
    ASSERT_EQ(cache_.size(), 70U);

    cache_.add_file("incoming", std::string(40, 'i'), 1);
    EXPECT_LE(cache_.size(), 100U);

    cache_.clear();
    cache_.set_max_size(10);
    cache_.add_file("first", std::string(6, 'a'), 1);
    EXPECT_EQ(cache_.size(), 6U);

    cache_.add_file("second", std::string(6, 'b'), 1);
    EXPECT_LE(cache_.size(), 10U);

    const size_t size_before_oversized_insert = cache_.size();
    cache_.add_file("oversized", std::string(11, 'c'), 1);
    EXPECT_EQ(cache_.size(), size_before_oversized_insert);

    cache_.clear();
    cache_.add_file("replacement", std::string(6, 'd'), 1);
    cache_.add_file("replacement", std::string(11, 'e'), 2);
    EXPECT_EQ(cache_.size(), 0U);
}

TEST_F(FileCacheTest, shrinking_the_limit_evicts_immediately)
{
    cache_.add_file("first", std::string(20, 'a'), 1);
    cache_.add_file("second", std::string(20, 'b'), 1);
    EXPECT_EQ(cache_.size(), 40U);

    cache_.set_max_size(20);
    EXPECT_LE(cache_.size(), 20U);

    cache_.set_max_size(0);
    EXPECT_EQ(cache_.size(), 0U);
}

TEST_F(FileCacheTest, stale_same_timestamp_entry_is_evicted)
{
    const std::string path = create_file("old");
    const std::time_t original_mtime = modification_time(path);
    cache_.add_file(path, "old", original_mtime);
    ASSERT_EQ(cache_.size(), 3U);

    overwrite_file(path, "changed");
    struct utimbuf original_times = {original_mtime, original_mtime};
    ASSERT_EQ(utime(path.c_str(), &original_times), 0);
    ASSERT_EQ(modification_time(path), original_mtime);

    std::string output;
    EXPECT_FALSE(cache_.get_file(path, output));
    EXPECT_EQ(cache_.size(), 0U);
}

TEST_F(FileCacheTest, deleted_entry_is_evicted_during_validation)
{
    const std::string path = create_file("content");
    cache_.add_file(path, "content", modification_time(path));
    ASSERT_EQ(cache_.size(), 7U);
    ASSERT_EQ(std::remove(path.c_str()), 0);

    EXPECT_FALSE(cache_.is_valid(path));
    EXPECT_EQ(cache_.size(), 0U);
}

TEST_F(FileCacheTest, disabled_cache_rejects_new_work)
{
    const std::string path = create_file("content");
    cache_.disable();
    cache_.add_file(path, "content", modification_time(path));
    EXPECT_EQ(cache_.size(), 0U);

    std::string output;
    EXPECT_FALSE(cache_.get_file(path, output));

    cache_.enable();
    cache_.add_file(path, "content", modification_time(path));
    EXPECT_TRUE(cache_.get_file(path, output));
    EXPECT_EQ(output, "content");
}

TEST_F(FileCacheTest, concurrent_configuration_and_access_preserve_the_bound)
{
    const std::string content = "concurrent-content";
    const std::string path = create_file(content);
    const std::time_t mtime = modification_time(path);
    std::atomic<bool> start(false);

    auto wait_for_start = [&start]() {
        while (!start.load(std::memory_order_acquire))
            std::this_thread::yield();
    };

    std::thread writer([&]() {
        wait_for_start();
        for (size_t i = 0; i < 2000; ++i)
            cache_.add_file(path, content, mtime);
    });

    std::thread reader([&]() {
        wait_for_start();
        std::string output;
        for (size_t i = 0; i < 2000; ++i)
        {
            cache_.get_file(path, output);
            cache_.is_valid(path);
            cache_.size();
        }
    });

    std::thread configurer([&]() {
        wait_for_start();
        for (size_t i = 0; i < 1000; ++i)
        {
            cache_.set_max_size(i % 64);
            if (i % 2 == 0)
                cache_.disable();
            else
                cache_.enable();
        }
    });

    start.store(true, std::memory_order_release);
    writer.join();
    reader.join();
    configurer.join();

    cache_.enable();
    cache_.set_max_size(32);
    cache_.add_file(path, content, mtime);
    EXPECT_LE(cache_.size(), 32U);

    std::string output;
    EXPECT_TRUE(cache_.get_file(path, output));
    EXPECT_EQ(output, content);
}
