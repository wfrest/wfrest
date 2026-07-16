#include <gtest/gtest.h>

#include <string>

#include "wfrest/CodeUtil.h"
#include "wfrest/StringPiece.h"
#include "wfrest/UriUtil.h"

using namespace wfrest;

TEST(UriUtil, preserves_everything_after_the_first_equals_sign)
{
    const auto fields = UriUtil::split_query(
        StringPiece("token=a=b=c&flag&empty=&=ignored&&tail=value"));

    ASSERT_EQ(fields.size(), 4U);
    EXPECT_EQ(fields.at("token"), "a=b=c");
    EXPECT_EQ(fields.at("flag"), "");
    EXPECT_EQ(fields.at("empty"), "");
    EXPECT_EQ(fields.at("tail"), "value");
}

TEST(UriUtil, decodes_after_structural_splitting)
{
    const auto fields = UriUtil::split_query(StringPiece(
        "message=hello+world&expr=a%26b%3Dc&encoded%20key=value%3D%3D"));

    ASSERT_EQ(fields.size(), 3U);
    EXPECT_EQ(fields.at("message"), "hello world");
    EXPECT_EQ(fields.at("expr"), "a&b=c");
    EXPECT_EQ(fields.at("encoded key"), "value==");
}

TEST(UriUtil, decoded_duplicate_keys_keep_the_first_value)
{
    const auto fields = UriUtil::split_query(
        StringPiece("%61=first&a=second&%2561=percent-a"));

    ASSERT_EQ(fields.size(), 2U);
    EXPECT_EQ(fields.at("a"), "first");
    EXPECT_EQ(fields.at("%61"), "percent-a");
}

TEST(UriUtil, malformed_escapes_are_literal_and_valid_nul_is_binary)
{
    const auto fields = UriUtil::split_query(
        StringPiece("bad=%ZZ&mixed=%2G&short=%2&percent=%&nul=%00"));

    EXPECT_EQ(fields.at("bad"), "%ZZ");
    EXPECT_EQ(fields.at("mixed"), "%2G");
    EXPECT_EQ(fields.at("short"), "%2");
    EXPECT_EQ(fields.at("percent"), "%");
    ASSERT_EQ(fields.at("nul").size(), 1U);
    EXPECT_EQ(fields.at("nul")[0], '\0');
}

TEST(CodeUtil, url_decode_accepts_only_complete_hexadecimal_escapes)
{
    EXPECT_EQ(CodeUtil::url_decode("%41%4a%4A+value"), "AJJ value");
    EXPECT_EQ(CodeUtil::url_decode("%GG%4%"), "%GG%4%");

    const std::string binary = CodeUtil::url_decode("a%00b");
    ASSERT_EQ(binary.size(), 3U);
    EXPECT_EQ(binary[0], 'a');
    EXPECT_EQ(binary[1], '\0');
    EXPECT_EQ(binary[2], 'b');
}
