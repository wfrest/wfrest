#include <cstdint>
#include <cstring>
#include <new>
#include <string>
#include <gtest/gtest.h>
#include "wfrest/HttpMsg.h"

using namespace wfrest;

namespace
{

std::string output_body(const protocol::HttpMessage &message)
{
    std::string body;
    EXPECT_TRUE(message.get_output_body_merged(body));
    return body;
}

} // namespace

TEST(HttpReqMove, initializes_default_and_wrapped_requests)
{
    HttpReq request;
    EXPECT_EQ(request.content_type(), CONTENT_TYPE_NONE);
    EXPECT_TRUE(request.body().empty());

    alignas(HttpReq) unsigned char storage[sizeof(HttpReq)];
    std::memset(storage, 0xA5, sizeof(storage));
    protocol::HttpRequest base;
    ASSERT_TRUE(base.set_method("POST"));
    ASSERT_TRUE(base.set_request_uri("/wrapped"));

    HttpReq *wrapped = new (storage) HttpReq(std::move(base));
    EXPECT_EQ(wrapped->content_type(), CONTENT_TYPE_NONE);
    EXPECT_TRUE(wrapped->body().empty());
    EXPECT_STREQ(wrapped->get_method(), "POST");
    EXPECT_STREQ(wrapped->get_request_uri(), "/wrapped");
    wrapped->~HttpReq();
}

TEST(HttpReqMove, transfers_all_derived_state)
{
    HttpReq source;
    ASSERT_TRUE(source.set_method("POST"));
    ASSERT_TRUE(source.set_request_uri("/source"));
    ASSERT_TRUE(source.add_header_pair(
        "Content-Type", "application/x-www-form-urlencoded"));
    ASSERT_TRUE(source.add_header_pair("Cookie", "session=abc"));
    source.fill_header_map();
    source.fill_content_type();
    source.body() = "name=value";
    EXPECT_EQ(source.form_kv().at("name"), "value");
    EXPECT_EQ(source.cookies().at("session"), "abc");
    source.set_route_match_path("tail");
    source.set_full_path("/route*");
    source.set_route_params({{"id", "42"}});
    source.set_query_params({{"page", "3"}});

    HttpReq moved(std::move(source));
    EXPECT_EQ(moved.content_type(), APPLICATION_URLENCODED);
    EXPECT_EQ(moved.body(), "name=value");
    EXPECT_EQ(moved.form_kv().at("name"), "value");
    EXPECT_EQ(moved.cookie("session"), "abc");
    EXPECT_EQ(moved.match_path(), "tail");
    EXPECT_EQ(moved.full_path(), "/route*");
    EXPECT_EQ(moved.param("id"), "42");
    EXPECT_EQ(moved.query("page"), "3");
    EXPECT_EQ(moved.header("Content-Type"),
              "application/x-www-form-urlencoded");

    source = HttpReq();
    source.body() = "reused";
    EXPECT_EQ(source.body(), "reused");
}

TEST(HttpReqMove, replaces_owned_cache_without_leaks)
{
    HttpReq target;
    target.body() = "initial";

    for (int i = 0; i < 100; ++i)
    {
        HttpReq source;
        source.body() = "body-" + std::to_string(i);
        target = std::move(source);
        EXPECT_EQ(target.body(), "body-" + std::to_string(i));
    }
}

TEST(HttpReqMove, self_move_preserves_state)
{
    HttpReq request;
    ASSERT_TRUE(request.set_method("PUT"));
    ASSERT_TRUE(request.set_request_uri("/self"));
    request.body() = "keep";
    request.set_route_params({{"id", "7"}});
    request.set_query_params({{"q", "value"}});

    HttpReq *request_alias = &request;
    request = std::move(*request_alias);

    EXPECT_STREQ(request.get_method(), "PUT");
    EXPECT_STREQ(request.get_request_uri(), "/self");
    EXPECT_EQ(request.body(), "keep");
    EXPECT_EQ(request.param("id"), "7");
    EXPECT_EQ(request.query("q"), "value");
}

TEST(HttpRespMove, initializes_user_data)
{
    alignas(HttpResp) unsigned char default_storage[sizeof(HttpResp)];
    std::memset(default_storage, 0xA5, sizeof(default_storage));
    HttpResp *response = new (default_storage) HttpResp;
    EXPECT_EQ(response->user_data, nullptr);
    response->~HttpResp();

    alignas(HttpResp) unsigned char wrapped_storage[sizeof(HttpResp)];
    std::memset(wrapped_storage, 0xA5, sizeof(wrapped_storage));
    protocol::HttpResponse base;
    ASSERT_TRUE(base.set_status_code("202"));
    HttpResp *wrapped = new (wrapped_storage) HttpResp(std::move(base));
    EXPECT_EQ(wrapped->user_data, nullptr);
    EXPECT_STREQ(wrapped->get_status_code(), "202");
    wrapped->~HttpResp();
}

TEST(HttpRespMove, transfers_and_replaces_state)
{
    HttpResp target;
    ASSERT_TRUE(target.set_status_code("400"));
    ASSERT_TRUE(target.append_output_body("old"));
    target.headers["X-State"] = "old";
    target.user_data = reinterpret_cast<void *>(static_cast<uintptr_t>(0x1));
    target.add_cookie(HttpCookie("old", "cookie"));

    HttpResp source;
    ASSERT_TRUE(source.set_status_code("201"));
    ASSERT_TRUE(source.append_output_body("created"));
    source.headers["X-State"] = "new";
    source.user_data = reinterpret_cast<void *>(static_cast<uintptr_t>(0x1234));
    source.add_cookie(HttpCookie("session", "abc"));

    target = std::move(source);
    EXPECT_STREQ(target.get_status_code(), "201");
    EXPECT_EQ(output_body(target), "created");
    EXPECT_EQ(target.headers.at("X-State"), "new");
    EXPECT_EQ(target.user_data,
              reinterpret_cast<void *>(static_cast<uintptr_t>(0x1234)));
    ASSERT_EQ(target.cookies().size(), 1U);
    EXPECT_EQ(target.cookies()[0].key(), "session");
    EXPECT_EQ(source.user_data, nullptr);

    HttpResp moved(std::move(target));
    EXPECT_STREQ(moved.get_status_code(), "201");
    EXPECT_EQ(output_body(moved), "created");
    EXPECT_EQ(moved.headers.at("X-State"), "new");
    EXPECT_EQ(moved.user_data,
              reinterpret_cast<void *>(static_cast<uintptr_t>(0x1234)));
    ASSERT_EQ(moved.cookies().size(), 1U);
    EXPECT_EQ(target.user_data, nullptr);
}

TEST(HttpRespMove, self_move_preserves_state)
{
    HttpResp response;
    ASSERT_TRUE(response.set_status_code("204"));
    ASSERT_TRUE(response.append_output_body("keep"));
    response.headers["X-State"] = "keep";
    response.user_data = reinterpret_cast<void *>(static_cast<uintptr_t>(0x1234));
    response.add_cookie(HttpCookie("name", "value"));

    HttpResp *response_alias = &response;
    response = std::move(*response_alias);

    EXPECT_STREQ(response.get_status_code(), "204");
    EXPECT_EQ(output_body(response), "keep");
    EXPECT_EQ(response.headers.at("X-State"), "keep");
    EXPECT_EQ(response.user_data,
              reinterpret_cast<void *>(static_cast<uintptr_t>(0x1234)));
    ASSERT_EQ(response.cookies().size(), 1U);
    EXPECT_EQ(response.cookies()[0].value(), "value");
}
