#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "wfrest/base64.h"

using namespace wfrest;

namespace
{

struct Base64Vector
{
    const char *plain;
    const char *encoded;
    const char *unpadded;
};

const Base64Vector kVectors[] = {
    {"", "", ""},
    {"f", "Zg==", "Zg"},
    {"fo", "Zm8=", "Zm8"},
    {"foo", "Zm9v", "Zm9v"},
    {"foob", "Zm9vYg==", "Zm9vYg"},
    {"fooba", "Zm9vYmE=", "Zm9vYmE"},
    {"foobar", "Zm9vYmFy", "Zm9vYmFy"}
};

} // namespace

TEST(Base64, matches_known_padded_and_unpadded_vectors)
{
    for (const Base64Vector& vector : kVectors)
    {
        SCOPED_TRACE(vector.plain);
        const std::string plain = vector.plain;
        EXPECT_EQ(Base64::encode(
                      reinterpret_cast<const unsigned char *>(plain.data()),
                      static_cast<unsigned int>(plain.size())),
                  vector.encoded);

        std::string decoded = "stale";
        EXPECT_TRUE(Base64::decode(vector.encoded, &decoded));
        EXPECT_EQ(decoded, plain);
        EXPECT_TRUE(Base64::decode(vector.unpadded, &decoded));
        EXPECT_EQ(decoded, plain);
        EXPECT_EQ(Base64::decode(vector.encoded), plain);
    }
}

TEST(Base64, round_trips_all_bytes_and_group_boundaries)
{
    std::string all_bytes;
    for (unsigned int byte = 0; byte <= 0xFFU; ++byte)
        all_bytes.push_back(static_cast<char>(byte));

    for (size_t size = 0; size <= all_bytes.size(); ++size)
    {
        SCOPED_TRACE(size);
        const std::string source = all_bytes.substr(0, size);
        std::string encoded;
        ASSERT_TRUE(Base64::encode(
            reinterpret_cast<const unsigned char *>(source.data()),
            source.size(),
            &encoded));
        std::string decoded;
        ASSERT_TRUE(Base64::decode(encoded, &decoded));
        EXPECT_EQ(decoded, source);
    }

    std::string long_source;
    long_source.reserve(100000);
    for (size_t i = 0; i < 100000; ++i)
        long_source.push_back(static_cast<char>(i & 0xFFU));
    std::string long_encoded;
    ASSERT_TRUE(Base64::encode(
        reinterpret_cast<const unsigned char *>(long_source.data()),
        long_source.size(),
        &long_encoded));
    std::string long_decoded;
    ASSERT_TRUE(Base64::decode(long_encoded, &long_decoded));
    EXPECT_EQ(long_decoded, long_source);
}

TEST(Base64, supports_checked_input_output_aliases)
{
    const std::string original = "raw input backed by output";
    std::string value = original;
    ASSERT_TRUE(Base64::encode(
        reinterpret_cast<const unsigned char *>(value.data()),
        value.size(),
        &value));
    EXPECT_NE(value, original);

    ASSERT_TRUE(Base64::decode(value, &value));
    EXPECT_EQ(value, original);
}

TEST(Base64, validates_null_boundaries)
{
    std::string output = "stale";
    EXPECT_FALSE(Base64::encode(nullptr, 1, &output));
    EXPECT_TRUE(output.empty());

    output = "stale";
    EXPECT_TRUE(Base64::encode(nullptr, 0, &output));
    EXPECT_TRUE(output.empty());

    EXPECT_FALSE(Base64::encode(nullptr, 0, nullptr));
    EXPECT_FALSE(Base64::decode("", nullptr));
    EXPECT_TRUE(Base64::encode(nullptr, 1).empty());
}

TEST(Base64, rejects_complete_malformed_input_without_prefixes)
{
    const std::vector<std::string> invalid = {
        "A", "AAAAA", "TWFu$admin", "TWFu\n", "T WFu", "TWFu\r\n",
        "TQ=evil",
        "TQ===", "=TQ=", "T=Q=", "AA=A", "-w==", "_w==",
        "AB==", "AB", "AAB=", "AAB",
        std::string("AA\0A", 4),
        std::string(1, static_cast<char>(0xFF))};

    for (const std::string& encoded : invalid)
    {
        SCOPED_TRACE(encoded);
        std::string output = "stale";
        EXPECT_FALSE(Base64::decode(encoded, &output));
        EXPECT_TRUE(output.empty());
        EXPECT_TRUE(Base64::decode(encoded).empty());
    }
}

TEST(Base64, accepts_canonical_final_bits)
{
    const std::vector<std::string> valid = {
        "AA==", "AA", "AAA=", "AAA", "/w==", "/w", "//8=", "//8"};

    for (const std::string& encoded : valid)
    {
        SCOPED_TRACE(encoded);
        std::string output;
        EXPECT_TRUE(Base64::decode(encoded, &output));
    }
}
