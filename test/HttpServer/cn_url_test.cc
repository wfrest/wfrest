#include "workflow/WFFacilities.h"
#include <gtest/gtest.h>
#include "wfrest/HttpServer.h"
#include "wfrest/ErrorCode.h"
#include "wfrest/CodeUtil.h"

using namespace wfrest;
using namespace protocol;

TEST(HttpServer, cn_url)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.GET(CodeUtil::url_encode("/您好"), [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("test");
    });

    EXPECT_TRUE(svr.track().start("127.0.0.1", 8888) == 0) << "http server start failed";
    svr.list_routes();
    WFHttpTask *client_task = WFTaskFactory::create_http_task("http://127.0.0.1:8888/您好", 4, 2, nullptr);
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        HttpResponse *resp = task->get_resp();

        const void *body;
        size_t body_len;

        resp->get_parsed_body(&body, &body_len);
        EXPECT_TRUE(strcmp("test", static_cast<const char *>(body)) == 0);
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
    svr.stop();
}
