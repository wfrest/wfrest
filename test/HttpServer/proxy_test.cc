#include "workflow/WFFacilities.h"
#include <gtest/gtest.h>
#include "wfrest/HttpServer.h"
#include "wfrest/ErrorCode.h"

using namespace wfrest;
using namespace protocol;

WFHttpTask *create_http_task(const std::string &path)
{
    return WFTaskFactory::create_http_task("http://127.0.0.1:8888/" + path, 4, 2, nullptr);
}

TEST(HttpServer, proxy)
{
    HttpServer svr;
    HttpServer proxy_svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.GET("/test", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("test");
    });

    proxy_svr.GET("/proxy", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Http("http://127.0.0.1:8887/test");
    });

    EXPECT_TRUE(svr.track().start("127.0.0.1", 8887) == 0) << "http server start failed";
    EXPECT_TRUE(proxy_svr.track().start("127.0.0.1", 8888) == 0) << "proxy http server start failed";

    WFHttpTask *client_task = create_http_task("proxy");
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        HttpRequest *req = task->get_req();
        HttpResponse *resp = task->get_resp();

        const void *body;
        size_t body_len;

        resp->get_parsed_body(&body, &body_len);
        // fprintf(stderr, "%s\n", static_cast<const char *>(body));
        EXPECT_TRUE(strcmp("test", static_cast<const char *>(body)) == 0);
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
    svr.stop();
    proxy_svr.stop();
}
