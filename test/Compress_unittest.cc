#include <gtest/gtest.h>

#include <string>

#include "wfrest/Compress.h"
#include "wfrest/ErrorCode.h"

using namespace wfrest;

namespace
{

std::string gzip_value(const std::string& source)
{
    std::string encoded;
    EXPECT_EQ(Compressor::gzip(&source, &encoded), StatusOK);
    return encoded;
}

void expect_round_trip(const std::string& source)
{
    const std::string encoded = gzip_value(source);
    std::string decoded;
    EXPECT_EQ(Compressor::ungzip(&encoded, &decoded), StatusOK);
    EXPECT_EQ(decoded, source);
}

} // namespace

TEST(Compressor, round_trips_short_and_long_text)
{
    expect_round_trip("WFREST compress : Just for test....");

    std::string long_text;
    for (size_t i = 0; i < 100000; ++i)
        long_text.append(std::to_string(i));
    expect_round_trip(long_text);
}

TEST(Compressor, round_trips_binary_and_empty_payloads)
{
    const char binary_bytes[] = {
        '\0', '\1', static_cast<char>(0xFF), 'a', '\0', 'z'};
    expect_round_trip(
        std::string(binary_bytes, sizeof(binary_bytes)));

    std::string encoded = "stale";
    EXPECT_EQ(Compressor::gzip(nullptr, 0, &encoded), StatusOK);
    EXPECT_FALSE(encoded.empty());

    std::string decoded = "stale";
    EXPECT_EQ(Compressor::ungzip(&encoded, &decoded), StatusOK);
    EXPECT_TRUE(decoded.empty());

    std::string empty;
    EXPECT_EQ(Compressor::gzip(&empty, &empty), StatusOK);
    EXPECT_FALSE(empty.empty());
    EXPECT_EQ(Compressor::ungzip(&empty, &empty), StatusOK);
    EXPECT_TRUE(empty.empty());
}

TEST(Compressor, supports_string_source_destination_aliases)
{
    const std::string original =
        "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string value = original;

    EXPECT_EQ(Compressor::gzip(&value, &value), StatusOK);
    EXPECT_NE(value, original);
    EXPECT_EQ(Compressor::ungzip(&value, &value), StatusOK);
    EXPECT_EQ(value, original);
}

TEST(Compressor, supports_raw_ranges_backed_by_destination)
{
    const std::string original =
        "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string value = original;

    EXPECT_EQ(Compressor::gzip(value.data(), value.size(), &value), StatusOK);
    EXPECT_EQ(Compressor::ungzip(value.data(), value.size(), &value), StatusOK);
    EXPECT_EQ(value, original);

    value = "prefix-payload-suffix";
    EXPECT_EQ(Compressor::gzip(value.data() + 7, 7, &value), StatusOK);
    std::string decoded;
    EXPECT_EQ(Compressor::ungzip(&value, &decoded), StatusOK);
    EXPECT_EQ(decoded, "payload");
}

TEST(Compressor, preserves_zlib_and_trailing_byte_compatibility)
{
    const std::string source = "zlib-wrapped payload";
    uLongf encoded_size = compressBound(static_cast<uLong>(source.size()));
    std::string encoded(static_cast<size_t>(encoded_size), '\0');
    ASSERT_EQ(compress2(
                  reinterpret_cast<Bytef *>(&encoded[0]),
                  &encoded_size,
                  reinterpret_cast<const Bytef *>(source.data()),
                  static_cast<uLong>(source.size()),
                  Z_BEST_SPEED),
              Z_OK);
    encoded.resize(static_cast<size_t>(encoded_size));
    encoded.append("trailing bytes");

    std::string decoded;
    EXPECT_EQ(Compressor::ungzip(&encoded, &decoded), StatusOK);
    EXPECT_EQ(decoded, source);
}

TEST(Compressor, validates_null_boundaries_and_clears_failures)
{
    const std::string source = "value";
    std::string destination = "stale";

    EXPECT_EQ(Compressor::gzip(
                  static_cast<const std::string *>(nullptr), &destination),
              StatusCompressError);
    EXPECT_TRUE(destination.empty());

    destination = "stale";
    EXPECT_EQ(Compressor::ungzip(
                  static_cast<const std::string *>(nullptr), &destination),
              StatusUncompressError);
    EXPECT_TRUE(destination.empty());

    destination = "stale";
    EXPECT_EQ(Compressor::gzip(nullptr, 1, &destination),
              StatusCompressError);
    EXPECT_TRUE(destination.empty());

    destination = "stale";
    EXPECT_EQ(Compressor::ungzip(nullptr, 1, &destination),
              StatusUncompressError);
    EXPECT_TRUE(destination.empty());

    EXPECT_EQ(Compressor::gzip(&source, nullptr), StatusCompressError);
    EXPECT_EQ(Compressor::gzip(source.data(), source.size(), nullptr),
              StatusCompressError);
    EXPECT_EQ(Compressor::ungzip(&source, nullptr), StatusUncompressError);
    EXPECT_EQ(Compressor::ungzip(source.data(), source.size(), nullptr),
              StatusUncompressError);

    destination = "stale";
    EXPECT_EQ(Compressor::ungzip(nullptr, 0, &destination), StatusOK);
    EXPECT_TRUE(destination.empty());
}

TEST(Compressor, rejects_malformed_and_truncated_streams)
{
    std::string destination = "stale";
    const std::string malformed = "not a gzip stream";
    EXPECT_EQ(Compressor::ungzip(&malformed, &destination),
              StatusUncompressError);
    EXPECT_TRUE(destination.empty());

    std::string truncated = gzip_value("payload");
    ASSERT_FALSE(truncated.empty());
    truncated.pop_back();
    destination = "stale";
    EXPECT_EQ(Compressor::ungzip(&truncated, &destination),
              StatusUncompressError);
    EXPECT_TRUE(destination.empty());

    EXPECT_EQ(Compressor::ungzip(&truncated, &truncated),
              StatusUncompressError);
    EXPECT_TRUE(truncated.empty());
}
