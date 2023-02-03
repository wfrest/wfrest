#include <gtest/gtest.h>
#include "wfrest/HttpServer.h"
#include "wfrest/Json.h"
#include "workflow/WFFacilities.h"

using namespace wfrest;
using namespace protocol;

WFHttpTask *create_http_task(const std::string &path)
{
    std::string url = "http://127.0.0.1:8888";
    if(path.front() == '/')
    {
        url += path;
    } else
    {
        url = url + "/" + path;
    }
    return WFTaskFactory::create_http_task(url, 4, 2, nullptr);
}

TEST(HttpServer, param)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(2);

    svr.GET("/user/{name}/match*", [](const HttpReq *req, HttpResp *resp)
    {
        const std::string &full_path = req->full_path();
        const std::string &current_path = req->current_path();
        Json json;
        json["full_path"] = full_path;
        json["current_path"] = current_path;
        resp->Json(json);
    });
    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

    WFHttpTask *client_task_1 = create_http_task("/user/{name}/match123");
    SeriesWork *series = Workflow::create_series_work(client_task_1, nullptr);

    client_task_1->set_callback([&wait_group](WFHttpTask *task)
    {
        HttpResponse *resp = task->get_resp();
        const void *body;
        size_t body_len;
        resp->get_parsed_body(&body, &body_len);

        Json json = Json::parse(static_cast<const char *>(body));
        EXPECT_EQ(json["full_path"].get<std::string>(), "/user/{name}/match*");
        EXPECT_EQ(json["current_path"].get<std::string>(), "/user/{name}/match123");
        wait_group.done();
    });

    WFHttpTask *client_task_2 = create_http_task("/user/{name}/match");
    series->push_back(client_task_2);
    client_task_2->set_callback([&wait_group](WFHttpTask *task)
    {
        HttpResponse *resp = task->get_resp();
        const void *body;
        size_t body_len;
        resp->get_parsed_body(&body, &body_len);

        Json json = Json::parse(static_cast<const char *>(body));
        EXPECT_EQ(json["full_path"].get<std::string>(), "/user/{name}/match*");
        EXPECT_EQ(json["current_path"].get<std::string>(), "/user/{name}/match");
        wait_group.done();
    });
    series->start();
    wait_group.wait();
    svr.stop();
}
