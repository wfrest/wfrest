#include <gtest/gtest.h>
#include "wfrest/HttpServer.h"
#include "workflow/WFFacilities.h"

using namespace wfrest;
using namespace protocol;

WFHttpTask *create_http_task(const std::string &path)
{
    return WFTaskFactory::create_http_task("http://127.0.0.1:8888/" + path, 4, 2, nullptr);
}

TEST(HttpServer, String_short_str)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.GET("/test", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("world\n");
    });
    svr.start(8888);

    WFHttpTask *client_task = create_http_task("test");
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        HttpRequest *req = task->get_req();
        HttpResponse *resp = task->get_resp();

        const void *body;
        size_t body_len;

        resp->get_parsed_body(&body, &body_len);
        EXPECT_TRUE(strcmp("world\n", static_cast<const char *>(body)) == 0);
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
}

std::string generate_long_str()
{
    std::string str;
    for (size_t i = 0; i < 100000; i++)
    {
        str.append(std::to_string(i));
    }
    return str;
}

TEST(HttpServer, String_long_str)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.GET("/test", [](const HttpReq *req, HttpResp *resp)
    {
        std::string str = generate_long_str();
        resp->String(std::move(str));
    });
    svr.start(8888);

    WFHttpTask *client_task = create_http_task("test");
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        HttpResponse *resp = task->get_resp();

        const void *body;
        size_t body_len;

        resp->get_parsed_body(&body, &body_len);
        std::string str = generate_long_str();
        EXPECT_TRUE(strcmp(str.c_str(), static_cast<const char *>(body)) == 0);
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}