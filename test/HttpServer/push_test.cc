#include <atomic>
#include <string>
#include <gtest/gtest.h>
#include "wfrest/HttpServer.h"
#include "workflow/WFFacilities.h"
#include "../ClientUtil.h"

using namespace wfrest;
using namespace protocol;

TEST(HttpServer, push_stops_after_terminal_chunk)
{
    const std::string condition = "wfrest_push_terminal_test";
    std::atomic<int> push_calls{0};
    std::atomic<int> error_calls{0};
    HttpServer server;
    WFFacilities::WaitGroup wait_group(2);

    server.GET("/push", [&](const HttpReq *, HttpResp *resp)
    {
        resp->add_header("Content-Type", "text/event-stream");
        resp->add_header("Cache-Control", "no-cache");
        resp->add_header("Connection", "keep-alive");
        resp->headers["X-Push"] = "safe\r\nX-Push-Injected: yes";
        resp->headers["Content-Length"] = "1";
        resp->headers["Transfer-Encoding"] = "identity";
        resp->Push(condition, [&](std::string &body)
        {
            if (++push_calls == 1)
                body = "hello";
        }, [&]
        {
            ++error_calls;
        });
    });

    ASSERT_EQ(server.start("127.0.0.1", 8888), 0);

    WFHttpTask *client = ClientUtil::create_http_task("push");
    client->set_callback([&](WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        HttpHeaderMap headers(task->get_resp());
        EXPECT_TRUE(headers.get("X-Push").empty());
        EXPECT_TRUE(headers.get("X-Push-Injected").empty());
        EXPECT_TRUE(headers.get("Content-Length").empty());
        const auto transfer_encodings =
            headers.get_strict("Transfer-Encoding");
        EXPECT_EQ(transfer_encodings.size(), 1U);
        if (!transfer_encodings.empty())
        {
            EXPECT_EQ(transfer_encodings.front(), "chunked");
        }
        const void *body = nullptr;
        size_t body_size = 0;
        const bool parsed = task->get_resp()->get_parsed_body(&body, &body_size);
        EXPECT_TRUE(parsed);
        if (parsed)
        {
            EXPECT_EQ(std::string(static_cast<const char *>(body), body_size),
                      "5\r\nhello\r\n0\r\n\r\n");
        }
        wait_group.done();
    });
    client->start();

    for (long milliseconds : {50L, 100L, 150L})
    {
        WFTimerTask *signal = WFTaskFactory::create_timer_task(
            0, milliseconds * 1000000, [condition](WFTimerTask *)
            {
                sse_signal(condition);
            });
        signal->start();
    }

    WFTimerTask *settle = WFTaskFactory::create_timer_task(
        0, 250000000, [&](WFTimerTask *)
        {
            wait_group.done();
        });
    settle->start();

    wait_group.wait();
    EXPECT_EQ(push_calls.load(), 2);
    EXPECT_EQ(error_calls.load(), 0);
    server.stop();
}
