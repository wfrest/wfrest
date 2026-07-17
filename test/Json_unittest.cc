#include <cerrno>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <unistd.h>

#include <gtest/gtest.h>

#include "wfrest/Json.h"

using namespace wfrest;

namespace
{

using FilePtr = std::unique_ptr<FILE, int (*)(FILE *)>;

bool write_all(int fd, const char *data, size_t size)
{
    size_t offset = 0;
    while (offset < size)
    {
        const ssize_t written = write(fd, data + offset, size - offset);
        if (written > 0)
        {
            offset += static_cast<size_t>(written);
            continue;
        }
        if (written < 0 && errno == EINTR)
            continue;
        return false;
    }
    return true;
}

std::string raw_nul_document()
{
    std::string input = "{\"prefix\":true}";
    input.push_back('\0');
    input.append("not-json");
    return input;
}

} // namespace

TEST(JsonParse, rejects_embedded_raw_nul)
{
    EXPECT_FALSE(Json::parse(raw_nul_document()).is_valid());
    EXPECT_TRUE(Json::parse("{\"escaped\":\"\\u0000\"}").is_valid());

    FilePtr stream(tmpfile(), fclose);
    ASSERT_NE(stream, nullptr);
    const std::string input = raw_nul_document();
    ASSERT_EQ(fwrite(input.data(), 1, input.size(), stream.get()),
              input.size());
    EXPECT_FALSE(Json::parse(stream.get()).is_valid());
}

TEST(JsonParse, rejects_null_empty_and_read_error_streams)
{
    EXPECT_FALSE(Json::parse(static_cast<FILE *>(nullptr)).is_valid());

    FilePtr empty(tmpfile(), fclose);
    ASSERT_NE(empty, nullptr);
    EXPECT_FALSE(Json::parse(empty.get()).is_valid());

    FilePtr directory(fopen(".", "r"), fclose);
    if (directory != nullptr)
    {
        EXPECT_FALSE(Json::parse(directory.get()).is_valid());
    }
}

TEST(JsonParse, rewinds_and_consumes_multichunk_seekable_file)
{
    const std::string value(20000, 'x');
    const std::string input = "{\"value\":\"" + value + "\"}";

    FilePtr stream(tmpfile(), fclose);
    ASSERT_NE(stream, nullptr);
    ASSERT_EQ(fwrite(input.data(), 1, input.size(), stream.get()),
              input.size());
    ASSERT_EQ(fseek(stream.get(), 100, SEEK_SET), 0);

    const Json parsed = Json::parse(stream.get());
    ASSERT_TRUE(parsed.is_valid());
    EXPECT_EQ(parsed["value"].get<std::string>(), value);
    EXPECT_NE(feof(stream.get()), 0);
    EXPECT_EQ(ftell(stream.get()), static_cast<long>(input.size()));
}

TEST(JsonParse, reads_nonseekable_stream_from_current_position)
{
    int descriptors[2];
    ASSERT_EQ(pipe(descriptors), 0);

    const std::string input = "{\"pipe\":true}";
    ASSERT_TRUE(write_all(descriptors[1], input.data(), input.size()));
    ASSERT_EQ(close(descriptors[1]), 0);

    FilePtr stream(fdopen(descriptors[0], "r"), fclose);
    ASSERT_NE(stream, nullptr);
    const Json parsed = Json::parse(stream.get());
    ASSERT_TRUE(parsed.is_valid());
    EXPECT_TRUE(parsed["pipe"].get<bool>());
    EXPECT_NE(feof(stream.get()), 0);
}

TEST(JsonParse, rejects_empty_nonseekable_stream)
{
    int descriptors[2];
    ASSERT_EQ(pipe(descriptors), 0);
    ASSERT_EQ(close(descriptors[1]), 0);

    FilePtr stream(fdopen(descriptors[0], "r"), fclose);
    ASSERT_NE(stream, nullptr);
    EXPECT_FALSE(Json::parse(stream.get()).is_valid());
}

TEST(JsonParse, rejects_embedded_nul_from_ifstream)
{
    const std::string path = "./wfrest-json-embedded-nul.tmp";
    const std::string input = raw_nul_document();
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(output.is_open());
        output.write(input.data(), static_cast<std::streamsize>(input.size()));
        ASSERT_TRUE(output.good());
    }

    std::ifstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    EXPECT_FALSE(Json::parse(stream).is_valid());
    stream.close();
    EXPECT_EQ(unlink(path.c_str()), 0);
}
