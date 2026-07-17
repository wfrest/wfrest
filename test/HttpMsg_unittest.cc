#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include "wfrest/HttpMsg.h"
#include "wfrest/HttpServerTask.h"

using namespace wfrest;

namespace
{

std::string output_body(const protocol::HttpMessage &message)
{
    std::string body;
    EXPECT_TRUE(message.get_output_body_merged(body));
    return body;
}

class ServerTaskHarness : public HttpServerTask
{
public:
    explicit ServerTaskHarness(ProcFunc& process) :
        HttpServerTask(nullptr, process)
    {}

    ~ServerTaskHarness()
    {
        this->target = nullptr;
    }

    void use_peer(CommTarget *peer)
    {
        this->target = peer;
    }

    CommMessageOut *finalize_response()
    {
        return this->message_out();
    }
};

class ScopedCommTarget
{
public:
    ScopedCommTarget(const struct sockaddr *addr, socklen_t addr_len) :
        initialized_(target_.init(addr, addr_len, 0, 0) == 0)
    {}

    ~ScopedCommTarget()
    {
        if (initialized_)
            target_.deinit();
    }

    bool initialized() const
    {
        return initialized_;
    }

    CommTarget *get()
    {
        return &target_;
    }

private:
    ScopedCommTarget(const ScopedCommTarget&) = delete;
    ScopedCommTarget& operator=(const ScopedCommTarget&) = delete;

    CommTarget target_;
    bool initialized_;
};

} // namespace

static_assert(std::is_same<
    decltype(std::declval<const HttpReq&>().default_query(
        std::declval<const std::string&>(), std::declval<std::string&>())),
    const std::string&>::value,
    "lvalue query fallbacks must keep reference semantics");

static_assert(std::is_same<
    decltype(std::declval<const HttpReq&>().default_query(
        std::declval<const std::string&>(), std::declval<std::string&&>())),
    std::string>::value,
    "temporary query fallbacks must return owned strings");

static_assert(std::is_same<
    decltype(std::declval<const HttpReq&>().default_query("key", "fallback")),
    std::string>::value,
    "literal query fallbacks must return owned strings");

TEST(HttpServerTaskPeer, rejects_unavailable_or_truncated_addresses)
{
    struct sockaddr truncated{};
    truncated.sa_family = AF_INET;
    ScopedCommTarget short_target(
        &truncated, sizeof(truncated.sa_family));
    ASSERT_TRUE(short_target.initialized());

    HttpServerTask::ProcFunc process = [](HttpTask *) {};
    ServerTaskHarness task(process);

    struct sockaddr_storage untouched;
    std::memset(&untouched, 0xA5, sizeof untouched);
    struct sockaddr_storage expected = untouched;
    socklen_t untouched_len = sizeof untouched;
    errno = 0;
    EXPECT_EQ(task.get_peer_addr(
        reinterpret_cast<struct sockaddr *>(&untouched), &untouched_len), -1);
    EXPECT_EQ(errno, ENOTCONN);
    EXPECT_EQ(untouched_len, sizeof untouched);
    EXPECT_EQ(std::memcmp(&untouched, &expected, sizeof untouched), 0);

    EXPECT_EQ(task.peer_addr(), "Unknown");
    EXPECT_EQ(task.peer_port(), 0);

    task.use_peer(short_target.get());
    EXPECT_EQ(task.peer_addr(), "Unknown");
    EXPECT_EQ(task.peer_port(), 0);
}

TEST(HttpServerTaskPeer, formats_ipv4_and_ipv6_addresses)
{
    struct sockaddr_in ipv4{};
    ipv4.sin_family = AF_INET;
    ipv4.sin_port = htons(8080);
    ASSERT_EQ(inet_pton(AF_INET, "127.0.0.1", &ipv4.sin_addr), 1);
    ScopedCommTarget ipv4_target(
        reinterpret_cast<const struct sockaddr *>(&ipv4), sizeof ipv4);
    ASSERT_TRUE(ipv4_target.initialized());

    struct sockaddr_in6 ipv6{};
    ipv6.sin6_family = AF_INET6;
    ipv6.sin6_port = htons(8443);
    ASSERT_EQ(inet_pton(AF_INET6, "::1", &ipv6.sin6_addr), 1);
    ScopedCommTarget ipv6_target(
        reinterpret_cast<const struct sockaddr *>(&ipv6), sizeof ipv6);
    ASSERT_TRUE(ipv6_target.initialized());

    HttpServerTask::ProcFunc process = [](HttpTask *) {};
    ServerTaskHarness task(process);

    task.use_peer(ipv4_target.get());
    EXPECT_EQ(task.peer_addr(), "127.0.0.1");
    EXPECT_EQ(task.peer_port(), 8080);

    task.use_peer(ipv6_target.get());
    EXPECT_EQ(task.peer_addr(), "::1");
    EXPECT_EQ(task.peer_port(), 8443);
}

TEST(HttpServerTaskStartLine, defaults_and_preserves_safe_custom_fields)
{
    HttpServerTask::ProcFunc process = [](HttpTask *) {};

    ServerTaskHarness defaults(process);
    ASSERT_NE(defaults.finalize_response(), nullptr);
    EXPECT_STREQ(defaults.get_resp()->get_http_version(), "HTTP/1.1");
    EXPECT_STREQ(defaults.get_resp()->get_status_code(), "200");
    EXPECT_STREQ(defaults.get_resp()->get_reason_phrase(), "OK");

    ServerTaskHarness custom(process);
    std::string custom_phrase = "Custom\tStatus ";
    custom_phrase.push_back(static_cast<char>(0x80));
    ASSERT_TRUE(custom.get_resp()->set_http_version("HTTP/1.0"));
    ASSERT_TRUE(custom.get_resp()->set_status_code("799"));
    ASSERT_TRUE(custom.get_resp()->set_reason_phrase(custom_phrase));
    ASSERT_NE(custom.finalize_response(), nullptr);
    EXPECT_STREQ(custom.get_resp()->get_http_version(), "HTTP/1.0");
    EXPECT_STREQ(custom.get_resp()->get_status_code(), "799");
    EXPECT_EQ(std::string(custom.get_resp()->get_reason_phrase()),
              custom_phrase);

    ServerTaskHarness empty_phrase(process);
    ASSERT_TRUE(empty_phrase.get_resp()->set_status_code("299"));
    ASSERT_TRUE(empty_phrase.get_resp()->set_reason_phrase(""));
    ASSERT_NE(empty_phrase.finalize_response(), nullptr);
    EXPECT_STREQ(empty_phrase.get_resp()->get_status_code(), "299");
    EXPECT_STREQ(empty_phrase.get_resp()->get_reason_phrase(), "");
}

TEST(HttpServerTaskStartLine, normalizes_invalid_start_line_fields)
{
    HttpServerTask::ProcFunc process = [](HttpTask *) {};

    ServerTaskHarness injected(process);
    ASSERT_TRUE(injected.get_resp()->set_http_version(
        "HTTP/1.1\r\nX-Version: injected"));
    ASSERT_TRUE(injected.get_resp()->set_status_code("299"));
    ASSERT_TRUE(injected.get_resp()->set_reason_phrase(
        "Custom\r\nX-Reason: injected"));
    ASSERT_NE(injected.finalize_response(), nullptr);
    EXPECT_STREQ(injected.get_resp()->get_http_version(), "HTTP/1.1");
    EXPECT_STREQ(injected.get_resp()->get_status_code(), "299");
    EXPECT_STREQ(injected.get_resp()->get_reason_phrase(), "Unknown");

    ServerTaskHarness control(process);
    ASSERT_TRUE(control.get_resp()->set_status_code("299"));
    ASSERT_TRUE(control.get_resp()->set_reason_phrase("Custom\x7f"));
    ASSERT_NE(control.finalize_response(), nullptr);
    EXPECT_STREQ(control.get_resp()->get_status_code(), "299");
    EXPECT_STREQ(control.get_resp()->get_reason_phrase(), "Unknown");

    const char *invalid_codes[] = {
        "99", "099", "+200", "200x", "200\r\nX-Code: injected",
        "999999999999999999999999"
    };
    for (const char *invalid_code : invalid_codes)
    {
        ServerTaskHarness invalid(process);
        ASSERT_TRUE(invalid.get_resp()->set_http_version("HTTP/2.0"));
        ASSERT_TRUE(invalid.get_resp()->set_status_code(invalid_code));
        ASSERT_TRUE(invalid.get_resp()->set_reason_phrase("Custom"));
        ASSERT_NE(invalid.finalize_response(), nullptr);
        EXPECT_STREQ(invalid.get_resp()->get_http_version(), "HTTP/1.1");
        EXPECT_STREQ(invalid.get_resp()->get_status_code(), "500");
        EXPECT_STREQ(invalid.get_resp()->get_reason_phrase(),
                     "Internal Server Error");
    }
}

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

TEST(HttpReqAccessors, converts_typed_params_without_exceptions)
{
    HttpReq request;
    const std::string int_min =
        std::to_string(std::numeric_limits<int>::min());
    const std::string int_max =
        std::to_string(std::numeric_limits<int>::max());
    const std::string size_max =
        std::to_string(std::numeric_limits<size_t>::max());
    const std::string embedded("12\0tail", 7);
    request.set_route_params({
        {"int", " -42"},
        {"int_min", int_min},
        {"int_max", int_max},
        {"size", "+17"},
        {"size_max", size_max},
        {"double", "1.25e2"},
        {"empty", ""},
        {"suffix", "12tail"},
        {"trailing", "12 "},
        {"overflow", "999999999999999999999999999999999"},
        {"negative_size", " -1"},
        {"size_overflow", size_max + "0"},
        {"nan", "nan"},
        {"infinity", "inf"},
        {"double_overflow", "1e9999"},
        {"double_underflow", "1e-9999"},
        {"embedded", embedded}
    });

    errno = EDOM;
    EXPECT_EQ(request.param<int>("int"), -42);
    EXPECT_EQ(request.param<int>("int_min"),
              std::numeric_limits<int>::min());
    EXPECT_EQ(request.param<int>("int_max"),
              std::numeric_limits<int>::max());
    EXPECT_EQ(request.param<int>("missing"), 0);
    EXPECT_EQ(request.param<int>("empty"), 0);
    EXPECT_EQ(request.param<int>("suffix"), 0);
    EXPECT_EQ(request.param<int>("trailing"), 0);
    EXPECT_EQ(request.param<int>("overflow"), 0);
    EXPECT_EQ(request.param<int>("embedded"), 0);
    EXPECT_EQ(errno, EDOM);

    errno = EAGAIN;
    EXPECT_EQ(request.param<size_t>("size"), 17U);
    EXPECT_EQ(request.param<size_t>("size_max"),
              std::numeric_limits<size_t>::max());
    EXPECT_EQ(request.param<size_t>("negative_size"), 0U);
    EXPECT_EQ(request.param<size_t>("size_overflow"), 0U);
    EXPECT_EQ(request.param<size_t>("embedded"), 0U);
    EXPECT_EQ(errno, EAGAIN);

    errno = ENOENT;
    EXPECT_DOUBLE_EQ(request.param<double>("double"), 125.0);
    EXPECT_DOUBLE_EQ(request.param<double>("suffix"), 0.0);
    EXPECT_DOUBLE_EQ(request.param<double>("nan"), 0.0);
    EXPECT_DOUBLE_EQ(request.param<double>("infinity"), 0.0);
    EXPECT_DOUBLE_EQ(request.param<double>("double_overflow"), 0.0);
    EXPECT_DOUBLE_EQ(request.param<double>("double_underflow"), 0.0);
    EXPECT_DOUBLE_EQ(request.param<double>("embedded"), 0.0);
    EXPECT_EQ(errno, ENOENT);
}

TEST(HttpReqAccessors, owns_temporary_default_query_fallbacks)
{
    HttpReq request;
    request.set_query_params({{"present", "stored"}});

    std::string fallback = "caller-owned";
    const std::string& missing_lvalue =
        request.default_query("missing", fallback);
    EXPECT_EQ(&missing_lvalue, &fallback);

    const std::string& present_lvalue =
        request.default_query("present", fallback);
    EXPECT_EQ(&present_lvalue, &request.query("present"));
    EXPECT_EQ(present_lvalue, "stored");

    const std::string& missing_temporary = request.default_query(
        "missing", std::string(4096, 'x'));
    EXPECT_EQ(missing_temporary.size(), 4096U);
    EXPECT_EQ(missing_temporary.front(), 'x');
    EXPECT_EQ(missing_temporary.back(), 'x');

    const std::string& missing_literal =
        request.default_query("missing", "literal");
    EXPECT_EQ(missing_literal, "literal");

    std::string present_owned = request.default_query(
        "present", std::string("unused"));
    request.set_query_params({{"present", "changed"}});
    EXPECT_EQ(present_owned, "stored");
}

TEST(HttpReqAccessors, current_path_is_safe_across_move_states)
{
    HttpReq request;
    EXPECT_TRUE(request.current_path().empty());

    ParsedURI parsed;
    ASSERT_EQ(URIParser::parse("http://example.test/path?query=1", parsed), 0);
    request.set_parsed_uri(std::move(parsed));
    EXPECT_EQ(request.current_path(), "/path");

    HttpReq moved(std::move(request));
    EXPECT_EQ(moved.current_path(), "/path");
    EXPECT_TRUE(request.current_path().empty());

    HttpReq assigned;
    assigned = std::move(moved);
    EXPECT_EQ(assigned.current_path(), "/path");
    EXPECT_TRUE(moved.current_path().empty());
}

TEST(HttpReqHeaders, refreshes_headers_and_cookie_cache)
{
    HttpReq request;
    EXPECT_TRUE(request.cookies().empty());

    ASSERT_TRUE(request.add_header_pair("X-Trace", "old"));
    ASSERT_TRUE(request.add_header_pair(
        "Cookie", "session=old; removed=value"));
    request.fill_header_map();

    EXPECT_EQ(request.header("x-trace"), "old");
    EXPECT_EQ(request.cookie("session"), "old");
    EXPECT_EQ(request.cookie("removed"), "value");

    ASSERT_TRUE(request.set_header_pair("X-Trace", "new"));
    ASSERT_TRUE(request.set_header_pair(
        "Cookie", "session=new; added=value"));
    request.fill_header_map();

    EXPECT_EQ(request.header("X-TRACE"), "new");
    EXPECT_EQ(request.header("Cookie"), "session=new; added=value");
    EXPECT_EQ(request.cookie("session"), "new");
    EXPECT_TRUE(request.cookie("removed").empty());
    EXPECT_EQ(request.cookie("added"), "value");
    EXPECT_EQ(request.cookies().size(), 2U);

    request.fill_header_map();
    EXPECT_EQ(request.header("X-Trace"), "new");
    EXPECT_EQ(request.cookie("session"), "new");
    EXPECT_EQ(request.cookie("added"), "value");
    EXPECT_EQ(request.cookies().size(), 2U);
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
