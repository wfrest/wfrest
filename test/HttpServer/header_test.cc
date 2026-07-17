#include <gtest/gtest.h>

#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#include "wfrest/HttpServer.h"
#include "wfrest/Json.h"
#include "wfrest/Timestamp.h"
#include "workflow/WFFacilities.h"
#include "../ClientUtil.h"

using namespace protocol;
using namespace wfrest;

namespace
{

class ScopedTimezone
{
public:
    explicit ScopedTimezone(const char *timezone)
    {
        const char *current = std::getenv("TZ");
        if (current != nullptr)
        {
            had_value_ = true;
            value_ = current;
        }

        (void)setenv("TZ", timezone, 1);
        tzset();
    }

    ~ScopedTimezone()
    {
        if (had_value_)
            (void)setenv("TZ", value_.c_str(), 1);
        else
            (void)unsetenv("TZ");
        tzset();
    }

private:
    bool had_value_ = false;
    std::string value_;
};

bool is_recent_utc_http_date(const std::string& value)
{
    const uint64_t now = Timestamp::now().micro_sec_since_epoch() /
                         Timestamp::k_micro_sec_per_sec;
    const uint64_t first = now > 5 ? now - 5 : 0;
    for (uint64_t second = first; second <= now + 5; ++second)
    {
        const Timestamp candidate(
            second * Timestamp::k_micro_sec_per_sec);
        if (candidate.to_utc_format_str(
                "%a, %d %b %Y %H:%M:%S GMT") == value)
        {
            return true;
        }
    }
    return false;
}

std::string response_body(WFHttpTask *task)
{
    const void *body = nullptr;
    size_t size = 0;
    if (!task->get_resp()->get_parsed_body(&body, &size) || size == 0)
        return "";
    return std::string(static_cast<const char *>(body), size);
}

} // namespace

TEST(HttpServer, validates_response_headers_and_framing)
{
    ScopedTimezone timezone("EST5");
    HttpServer server;
    WFFacilities::WaitGroup wait_group(8);

    server.GET("/sanitize", [](const HttpReq *, HttpResp *resp)
    {
        resp->add_header("X-Safe", "yes");
        resp->add_header("X-User", "safe\r\nX-Injected: yes");
        resp->add_header("Bad Name", "value");
        resp->headers["X-Direct"] = "safe\r\nX-Direct-Injected: yes";
        resp->headers["Bad:Direct"] = "value";
        resp->String("body");
    });
    server.GET("/framing", [](const HttpReq *, HttpResp *resp)
    {
        resp->headers["Content-Length"] = "1";
        resp->headers["Transfer-Encoding"] = "chunked";
        resp->String("abcdef");
    });
    server.GET("/redirect", [](const HttpReq *, HttpResp *resp)
    {
        resp->add_header("Location", "/previous");
        resp->Redirect("/safe\r\nX-Redirected: yes", 302);
    });
    server.GET("/json", [](const HttpReq *, HttpResp *resp)
    {
        resp->add_header("Content-Type", "text/plain");
        Json value;
        value["ok"] = true;
        resp->Json(value);
    });
    server.GET("/problem", [](const HttpReq *, HttpResp *resp)
    {
        resp->add_header("Content-Type",
                         "application/problem+json; charset=utf-8");
        Json value;
        value["problem"] = true;
        resp->Json(value);
    });
    server.GET("/start-line", [](const HttpReq *, HttpResp *resp)
    {
        resp->set_status_code("299");
        resp->set_reason_phrase("Custom\r\nX-Start-Line-Injected: yes");
        resp->String("safe");
    });
    ASSERT_EQ(server.start("127.0.0.1", 8888), 0);

    WFHttpTask *sanitize = ClientUtil::create_http_task("sanitize");
    sanitize->set_callback([&](WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        HttpHeaderMap headers(task->get_resp());
        EXPECT_EQ(headers.get("X-Safe"), "yes");
        EXPECT_TRUE(is_recent_utc_http_date(headers.get("Date")));
        EXPECT_TRUE(headers.get("X-Injected").empty());
        EXPECT_TRUE(headers.get("X-Direct-Injected").empty());
        EXPECT_TRUE(headers.get("Bad:Direct").empty());
        EXPECT_EQ(response_body(task), "body");
        wait_group.done();
    });
    sanitize->start();

    WFHttpTask *framing = ClientUtil::create_http_task("framing");
    framing->set_callback([&](WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        HttpHeaderMap headers(task->get_resp());
        EXPECT_EQ(headers.get("Content-Length"), "6");
        EXPECT_TRUE(headers.get("Transfer-Encoding").empty());
        EXPECT_EQ(response_body(task), "abcdef");
        wait_group.done();
    });
    framing->start();

    WFHttpTask *redirect = WFTaskFactory::create_http_task(
        "http://127.0.0.1:8888/redirect", 0, 2, nullptr);
    redirect->set_callback([&](WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        HttpHeaderMap headers(task->get_resp());
        EXPECT_STREQ(task->get_resp()->get_status_code(), "302");
        EXPECT_TRUE(headers.get("Location").empty());
        EXPECT_TRUE(headers.get("X-Redirected").empty());
        wait_group.done();
    });
    redirect->start();

    WFHttpTask *json = ClientUtil::create_http_task("json");
    json->set_callback([&](WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        HttpHeaderMap headers(task->get_resp());
        EXPECT_EQ(headers.get("Content-Type"), "application/json");
        EXPECT_TRUE(Json::parse(response_body(task)).is_valid());
        wait_group.done();
    });
    json->start();

    WFHttpTask *problem = ClientUtil::create_http_task("problem");
    problem->set_callback([&](WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        HttpHeaderMap headers(task->get_resp());
        EXPECT_EQ(headers.get("Content-Type"),
                  "application/problem+json; charset=utf-8");
        EXPECT_TRUE(Json::parse(response_body(task)).is_valid());
        wait_group.done();
    });
    problem->start();

    WFHttpTask *start_line = ClientUtil::create_http_task("start-line");
    start_line->set_callback([&](WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        EXPECT_STREQ(task->get_resp()->get_status_code(), "299");
        EXPECT_STREQ(task->get_resp()->get_reason_phrase(), "Unknown");
        HttpHeaderMap headers(task->get_resp());
        EXPECT_TRUE(headers.get("X-Start-Line-Injected").empty());
        EXPECT_EQ(response_body(task), "safe");
        wait_group.done();
    });
    start_line->start();

    WFHttpTask *keep_alive_max = ClientUtil::create_http_task("sanitize");
    EXPECT_TRUE(keep_alive_max->get_req()->add_header_pair(
        "Connection", "Keep-Alive"));
    EXPECT_TRUE(keep_alive_max->get_req()->add_header_pair(
        "Keep-Alive", "max=1"));
    keep_alive_max->set_callback([&](WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        HttpHeaderMap headers(task->get_resp());
        EXPECT_EQ(headers.get("Connection"), "close");
        wait_group.done();
    });
    keep_alive_max->start();

    WFHttpTask *keep_alive_malformed =
        ClientUtil::create_http_task("sanitize");
    EXPECT_TRUE(keep_alive_malformed->get_req()->add_header_pair(
        "Connection", "Keep-Alive"));
    EXPECT_TRUE(keep_alive_malformed->get_req()->add_header_pair(
        "Keep-Alive", "timeout=garbage"));
    keep_alive_malformed->set_callback([&](WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        HttpHeaderMap headers(task->get_resp());
        EXPECT_EQ(headers.get("Connection"), "Keep-Alive");
        wait_group.done();
    });
    keep_alive_malformed->start();

    wait_group.wait();
    server.stop();
}
