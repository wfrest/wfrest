#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

#include "wfrest/HttpMsg.h"
#include "workflow/HttpUtil.h"
#include "../src/core/HttpHeaderUtil.h"

using namespace wfrest;

TEST(HttpHeaderUtil, validates_names_and_values_by_byte)
{
    using detail::is_valid_response_header_name;
    using detail::is_valid_response_header_value;

    EXPECT_TRUE(is_valid_response_header_name(
        "AZaz09!#$%&'*+-.^_`|~"));
    for (const std::string &invalid : {
             std::string(), std::string("Bad Name"), std::string("Bad\tName"),
             std::string("Bad:Name"), std::string("Bad\rName"),
             std::string("Bad\nName"), std::string("Bad\0Name", 8),
             std::string("Bad\x7fName", 8), std::string("X-\x80", 3)})
    {
        EXPECT_FALSE(is_valid_response_header_name(invalid));
    }

    EXPECT_TRUE(is_valid_response_header_value(""));
    EXPECT_TRUE(is_valid_response_header_value("visible\tvalue"));
    EXPECT_TRUE(is_valid_response_header_value(std::string("obs-\x80", 5)));
    for (int byte = 0; byte < 32; ++byte)
    {
        if (byte == '\t')
            continue;
        EXPECT_FALSE(is_valid_response_header_value(
            std::string(1, static_cast<char>(byte))));
    }
    EXPECT_FALSE(is_valid_response_header_value(std::string(1, '\x7f')));
}

TEST(HttpHeaderUtil, removes_invalid_and_framework_managed_entries)
{
    std::map<std::string, std::string> headers = {
        {"X-Safe", "value"},
        {"Bad Name", "value"},
        {"X-Bad", "safe\r\nX-Injected: yes"},
        {"content-length", "1"},
        {"TRANSFER-ENCODING", "chunked"}
    };

    detail::sanitize_application_response_headers(&headers);
    ASSERT_EQ(headers.size(), 1U);
    EXPECT_EQ(headers.at("X-Safe"), "value");
}

TEST(HttpHeaderUtil, recognizes_json_media_types)
{
    using detail::is_json_response_content_type;
    EXPECT_TRUE(is_json_response_content_type("application/json"));
    EXPECT_TRUE(is_json_response_content_type(
        " Application/Problem+JSON \t; charset=utf-8"));
    EXPECT_TRUE(is_json_response_content_type("text/json; charset=utf-8"));

    EXPECT_FALSE(is_json_response_content_type(""));
    EXPECT_FALSE(is_json_response_content_type("charset=utf-8"));
    EXPECT_FALSE(is_json_response_content_type("text/plain"));
    EXPECT_FALSE(is_json_response_content_type("application/json/text"));
    EXPECT_FALSE(is_json_response_content_type("application/json text"));
    EXPECT_FALSE(is_json_response_content_type(
        "application/json\r\nX-Injected: yes"));
}

TEST(HttpHeaderUtil, response_wrappers_reject_unsafe_and_framing_fields)
{
    HttpResp response;
    response.add_header("X-Safe", "first");
    response.add_header("X-Safe", "bad\r\nX-Injected: yes");
    response.add_header("Bad Name", "value");
    response.add_header("Content-Length", "1");
    ASSERT_EQ(response.headers.size(), 1U);
    EXPECT_EQ(response.headers.at("X-Safe"), "first");

    EXPECT_TRUE(response.add_header_pair("X-Low", "value"));
    EXPECT_TRUE(response.set_header_pair("X-Low", "updated"));
    EXPECT_FALSE(response.add_header_pair("X-Bad", "value\nnext"));
    EXPECT_FALSE(response.set_header_pair("Transfer-Encoding", "chunked"));

    protocol::HttpHeaderMap parsed(&response);
    EXPECT_EQ(parsed.get("X-Low"), "updated");
    EXPECT_TRUE(parsed.get("X-Bad").empty());
    EXPECT_TRUE(parsed.get("Transfer-Encoding").empty());
}
