#include <gtest/gtest.h>
#include <vector>
#include "wfrest/HttpContent.h"
#include "wfrest/StringPiece.h"
#include "wfrest/UriUtil.h"

using namespace wfrest;

namespace
{

std::string multipart_body(const std::string &boundary,
                           const std::string &name,
                           const std::string &value,
                           const std::string &closing = "--\r\n")
{
    std::string body = "--" + boundary;
    body += "\r\nContent-Disposition: form-data; name=\"" + name;
    body += "\"\r\n\r\n";
    body += value;
    body += "\r\n--" + boundary + closing;
    return body;
}

std::string multipart_disposition_body(const std::string &boundary,
                                       const std::string &disposition,
                                       const std::string &value = "value")
{
    return "--" + boundary +
           "\r\nContent-Disposition: " + disposition +
           "\r\n\r\n" + value + "\r\n--" + boundary + "--\r\n";
}

} // namespace

TEST(Urlencode, parse_post_kv)
{
    const std::string payload =
        "token=a=b=c&message=hello+world&expr=a%26b%3Dc&"
        "flag&empty=&%61=first&a=second&bad=%ZZ&nul=%00";

    const auto form = Urlencode::parse_post_kv(StringPiece(payload));
    const auto query = UriUtil::split_query(StringPiece(payload));

    EXPECT_EQ(form, query);
    ASSERT_EQ(form.size(), 8U);
    EXPECT_EQ(form.at("token"), "a=b=c");
    EXPECT_EQ(form.at("message"), "hello world");
    EXPECT_EQ(form.at("expr"), "a&b=c");
    EXPECT_EQ(form.at("flag"), "");
    EXPECT_EQ(form.at("empty"), "");
    EXPECT_EQ(form.at("a"), "first");
    EXPECT_EQ(form.at("bad"), "%ZZ");
    ASSERT_EQ(form.at("nul").size(), 1U);
    EXPECT_EQ(form.at("nul")[0], '\0');

}

TEST(MultiPartForm, parses_only_complete_bodies)
{
    MultiPartForm parser;
    parser.set_boundary("abc");

    const std::string value("binary\0data\r\n--abx", 18);
    const Form valid = parser.parse_multipart(
        StringPiece(multipart_body("abc", "field", value)));
    ASSERT_EQ(valid.size(), 1U);
    EXPECT_EQ(valid.at("field").first, "");
    EXPECT_EQ(valid.at("field").second, value);

    const std::string multiple =
        "--abc\r\nContent-Disposition: form-data; name=\"first\"\r\n"
        "\r\none\r\n--abc\r\n"
        "Content-Disposition: form-data; name=\"second\"\r\n"
        "\r\ntwo\r\n--abc--\r\n";
    const Form fields = parser.parse_multipart(StringPiece(multiple));
    ASSERT_EQ(fields.size(), 2U);
    EXPECT_EQ(fields.at("first").second, "one");
    EXPECT_EQ(fields.at("second").second, "two");

    EXPECT_TRUE(parser.parse_multipart(StringPiece(
        multipart_body("abc", "field", "value", "X\r\n"))).empty());
    EXPECT_TRUE(parser.parse_multipart(StringPiece(
        multipart_body("abc", "field", "value", "\r\n"))).empty());

    const std::string truncated =
        "--abc\r\nContent-Disposition: form-data; name=\"field\"\r\n"
        "\r\nvalue";
    EXPECT_TRUE(parser.parse_multipart(StringPiece(truncated)).empty());
}

TEST(MultiPartForm, rejects_invalid_boundaries)
{
    const std::string valid_body = multipart_body("abc", "field", "value");
    MultiPartForm parser;

    EXPECT_TRUE(parser.parse_multipart(StringPiece(valid_body)).empty());

    parser.set_boundary("");
    EXPECT_TRUE(parser.parse_multipart(StringPiece(valid_body)).empty());

    parser.set_boundary("abc");
    parser.set_boundary("abc;");
    EXPECT_TRUE(parser.parse_multipart(StringPiece(valid_body)).empty());

    parser.set_boundary("abc ");
    EXPECT_TRUE(parser.parse_multipart(StringPiece(valid_body)).empty());

    parser.set_boundary(std::string(71, 'a'));
    EXPECT_TRUE(parser.parse_multipart(StringPiece(valid_body)).empty());
}

TEST(MultiPartForm, parses_structural_content_disposition)
{
    MultiPartForm parser;
    parser.set_boundary("abc");

    Form form = parser.parse_multipart(StringPiece(
        multipart_disposition_body(
            "abc",
            "FORM-DATA ; NAME = upload ; "
            "FILENAME = \"a=b;c.txt\"; filename*=UTF-8''ignored")));
    ASSERT_EQ(form.size(), 1U);
    EXPECT_EQ(form.at("upload").first, "a=b;c.txt");
    EXPECT_EQ(form.at("upload").second, "value");

    form = parser.parse_multipart(StringPiece(
        multipart_disposition_body(
            "abc",
            "form-data; name=item; filename=\"a\\\"b\\\\c.txt\"; "
            "filenamex=ignored")));
    ASSERT_EQ(form.size(), 1U);
    EXPECT_EQ(form.at("item").first, "a\"b\\c.txt");

    form = parser.parse_multipart(StringPiece(
        multipart_disposition_body(
            "abc", "form-data; name=\"up=load;field\"; filename=\"\"")));
    ASSERT_EQ(form.size(), 1U);
    EXPECT_TRUE(form.at("up=load;field").first.empty());
}

TEST(MultiPartForm, rejects_invalid_content_disposition_metadata)
{
    MultiPartForm parser;
    parser.set_boundary("abc");

    std::string nul_disposition = "form-data; name=\"bad";
    nul_disposition.push_back('\0');
    nul_disposition += "value\"";
    const std::vector<std::string> invalid = {
        "form-data; namex=spoofed",
        "form-data; filenamex=spoofed",
        "attachment; name=item",
        "form-data; filename=file.txt",
        "form-data; name=\"\"",
        "form-data; name=one; name=two",
        "form-data; name=item; filename=one; filename=two",
        "form-data; name",
        "form-data; name=",
        "form-data; name=item;",
        "form-data; name=\"item\"junk",
        "form-data; name=\"unterminated",
        "form-data; name=\"item\"; filename=\"trailing\\",
        "form-data; na(me=item",
        nul_disposition,
        std::string("form-data; name=\"bad") + '\x7f' + "value\""
    };

    for (const std::string &disposition : invalid)
    {
        EXPECT_TRUE(parser.parse_multipart(StringPiece(
            multipart_disposition_body("abc", disposition))).empty())
            << disposition;
    }

    const std::string duplicate_header =
        "--abc\r\nContent-Disposition: form-data; name=first\r\n"
        "Content-Disposition: form-data; name=second\r\n"
        "\r\nvalue\r\n--abc--\r\n";
    EXPECT_TRUE(parser.parse_multipart(StringPiece(duplicate_header)).empty());
}

TEST(MultiPartForm, isolates_invalid_disposition_between_parts)
{
    MultiPartForm parser;
    parser.set_boundary("abc");

    const std::string invalid_then_valid =
        "--abc\r\nContent-Disposition: form-data; namex=bad\r\n"
        "\r\nbad-data\r\n--abc\r\n"
        "Content-Disposition: form-data; name=good\r\n"
        "\r\ngood-data\r\n--abc--\r\n";
    Form form = parser.parse_multipart(StringPiece(invalid_then_valid));
    ASSERT_EQ(form.size(), 1U);
    EXPECT_EQ(form.at("good").second, "good-data");

    const std::string valid_then_invalid =
        "--abc\r\nContent-Disposition: form-data; name=good\r\n"
        "\r\ngood-data\r\n--abc\r\n"
        "Content-Disposition: attachment; name=bad\r\n"
        "\r\nbad-data\r\n--abc--\r\n";
    form = parser.parse_multipart(StringPiece(valid_then_invalid));
    ASSERT_EQ(form.size(), 1U);
    EXPECT_EQ(form.at("good").second, "good-data");
}

TEST(MultiPartParser, rejects_invalid_initialization)
{
    multipart_parser_settings settings = {};
    EXPECT_EQ(multipart_parser_init(nullptr, &settings), nullptr);
    EXPECT_EQ(multipart_parser_init("", &settings), nullptr);
    EXPECT_EQ(multipart_parser_init("abc", nullptr), nullptr);
    EXPECT_EQ(multipart_parser_execute(nullptr, "body", 4), 0U);
    EXPECT_EQ(multipart_parser_get_data(nullptr), nullptr);
    multipart_parser_set_data(nullptr, nullptr);
    multipart_parser_free(nullptr);

    multipart_parser *parser = multipart_parser_init("abc", &settings);
    ASSERT_NE(parser, nullptr);
    EXPECT_EQ(multipart_parser_get_data(parser), nullptr);
    EXPECT_EQ(multipart_parser_execute(parser, nullptr, 1), 0U);
    multipart_parser_free(parser);
}
