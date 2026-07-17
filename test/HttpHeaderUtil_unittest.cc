#include <gtest/gtest.h>

#include <map>
#include <limits>
#include <string>
#include <vector>

#include "wfrest/HttpMsg.h"
#include "workflow/HttpUtil.h"
#include "../src/core/HttpHeaderUtil.h"
#include "../src/core/HttpKeepAliveUtil.h"

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

TEST(HttpKeepAliveUtil, accepts_only_complete_unsigned_decimal_values)
{
    using detail::resolve_keep_alive_timeout;
    const int configured = 60000;

    EXPECT_EQ(resolve_keep_alive_timeout("timeout=5", 0, configured), 5000);
    EXPECT_EQ(resolve_keep_alive_timeout(
                  " extension=value, \tTiMeOuT \t= \t5 \t", 0, configured),
              5000);
    EXPECT_EQ(resolve_keep_alive_timeout(
                  "timeout=bad, timeout=7", 0, configured),
              7000);
    EXPECT_EQ(resolve_keep_alive_timeout(
                  "timeout=5, timeout=1", 0, configured),
              5000);

    const std::vector<std::string> malformed = {
        "timeout=", "timeout=-1", "timeout=+1", "timeout=1.5",
        "timeout=5junk", "timeout=5=6", "timeout=1 0"
    };
    for (const std::string &header : malformed)
    {
        EXPECT_EQ(resolve_keep_alive_timeout(header, 0, configured),
                  configured) << header;
    }

    std::string with_nul = "timeout=5";
    with_nul.push_back('\0');
    with_nul += "junk";
    EXPECT_EQ(resolve_keep_alive_timeout(with_nul, 0, configured),
              configured);

    const std::string huge(10000, '9');
    EXPECT_EQ(resolve_keep_alive_timeout(
                  "timeout=" + huge, 0, 300000),
              300000);
    EXPECT_EQ(resolve_keep_alive_timeout(
                  "timeout=" + huge + "x", 0, configured),
              configured);
}

TEST(HttpKeepAliveUtil, request_timeout_never_extends_server_policy)
{
    using detail::resolve_keep_alive_timeout;

    EXPECT_EQ(resolve_keep_alive_timeout("", 0, 0), 0);
    EXPECT_EQ(resolve_keep_alive_timeout("", 0, 5000), 5000);
    EXPECT_EQ(resolve_keep_alive_timeout("", 0, -1), 300000);
    EXPECT_EQ(resolve_keep_alive_timeout("", 0, 300001), 300000);

    EXPECT_EQ(resolve_keep_alive_timeout("timeout=0", 0, 60000), 0);
    EXPECT_EQ(resolve_keep_alive_timeout("timeout=5", 0, 60000), 5000);
    EXPECT_EQ(resolve_keep_alive_timeout("timeout=60", 0, 60000), 60000);
    EXPECT_EQ(resolve_keep_alive_timeout("timeout=300", 0, 5000), 5000);
    EXPECT_EQ(resolve_keep_alive_timeout("timeout=999999", 0, 5000),
              5000);
    EXPECT_EQ(resolve_keep_alive_timeout("timeout=-1", 0, 5000), 5000);
}

TEST(HttpKeepAliveUtil, max_uses_one_based_request_ordinal)
{
    using detail::resolve_keep_alive_timeout;
    const int configured = 60000;

    EXPECT_EQ(resolve_keep_alive_timeout("max=0", 0, configured), 0);
    EXPECT_EQ(resolve_keep_alive_timeout("max=1", 0, configured), 0);
    EXPECT_EQ(resolve_keep_alive_timeout("max=2", 0, configured),
              configured);
    EXPECT_EQ(resolve_keep_alive_timeout("max=5", 4, configured), 0);
    EXPECT_EQ(resolve_keep_alive_timeout("max=6", 4, configured),
              configured);
    EXPECT_EQ(resolve_keep_alive_timeout("max=1", -1, configured), 0);
    EXPECT_EQ(resolve_keep_alive_timeout("max=2", -1, configured),
              configured);

    EXPECT_EQ(resolve_keep_alive_timeout(
                  "max=bad, max=2", 0, configured),
              configured);
    EXPECT_EQ(resolve_keep_alive_timeout(
                  "max=2, max=1", 0, configured),
              configured);
    EXPECT_EQ(resolve_keep_alive_timeout(
                  "max=18446744073709551615",
                  std::numeric_limits<long long>::max(),
                  configured),
              configured);
    EXPECT_EQ(resolve_keep_alive_timeout(
                  "max=9223372036854775808",
                  std::numeric_limits<long long>::max(),
                  configured),
              0);
}

TEST(HttpKeepAliveUtil, combines_timeout_and_max_independently)
{
    using detail::resolve_keep_alive_timeout;

    EXPECT_EQ(resolve_keep_alive_timeout(
                  "timeout=5, max=2", 0, 60000),
              5000);
    EXPECT_EQ(resolve_keep_alive_timeout(
                  "max=1, timeout=5", 0, 60000),
              0);
    EXPECT_EQ(resolve_keep_alive_timeout(
                  "unknown, max=bad, timeout=bad", 0, 60000),
              60000);
}
