#include "workflow/WFFacilities.h"
#include "workflow/Workflow.h"

#include <gtest/gtest.h>

#include "wfrest/HttpServer.h"
#include "wfrest/ErrorCode.h"

using namespace wfrest;
using namespace protocol;

WFHttpTask *create_http_task(const std::string &path)
{
    return WFTaskFactory::create_http_task("http://127.0.0.1:8888/" + path, 4, 2, nullptr);
}

void Factorial(int n, HttpResp *resp)
{
    unsigned long long factorial = 1;
    for(int i = 1; i <= n; i++)
    {
        factorial *= i;
    }
    resp->String(std::to_string(factorial));
}

TEST(HttpServer, String_short_str)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.GET("/test", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Compute(1, Factorial, 10, resp);
    });

    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

    WFHttpTask *client_task = create_http_task("test");
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        const void *body;
        size_t body_len;
        task->get_resp()->get_parsed_body(&body, &body_len);
        EXPECT_TRUE(strcmp("3628800", static_cast<const char *>(body)) == 0);
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
    svr.stop();
}
