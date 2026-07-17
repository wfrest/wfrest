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
    WFFacilities::WaitGroup wait_group(5);

    svr.GET("/user/{name}/match*", [](const HttpReq *req, HttpResp *resp)
    {
        const std::string &full_path = req->full_path();
        const std::string &current_path = req->current_path();
        Json json;
        json["full_path"] = full_path;
        json["current_path"] = current_path;
        resp->Json(json);
    });
    svr.GET("/query", [](const HttpReq *req, HttpResp *resp)
    {
        Json json;
        json["token"] = req->query("token");
        json["message"] = req->query("message");
        json["expr"] = req->query("expr");
        json["bad"] = req->query("bad");
        resp->Json(json);
    });
    svr.GET("/number/{value}", [](const HttpReq *req, HttpResp *resp)
    {
        Json json;
        json["int"] = req->param<int>("value");
        json["size"] = req->param<size_t>("value");
        json["double"] = req->param<double>("value");
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

    WFHttpTask *client_task_3 = create_http_task(
        "/query?token=a=b=c&message=hello+world&expr=a%26b%3Dc&bad=%ZZ");
    series->push_back(client_task_3);
    client_task_3->set_callback([&wait_group](WFHttpTask *task)
    {
        const void *body = nullptr;
        size_t body_len = 0;
        task->get_resp()->get_parsed_body(&body, &body_len);
        Json json = Json::parse(std::string(static_cast<const char *>(body), body_len));
        EXPECT_EQ(json["token"].get<std::string>(), "a=b=c");
        EXPECT_EQ(json["message"].get<std::string>(), "hello world");
        EXPECT_EQ(json["expr"].get<std::string>(), "a&b=c");
        EXPECT_EQ(json["bad"].get<std::string>(), "%ZZ");
        wait_group.done();
    });

    WFHttpTask *invalid_number = create_http_task("/number/not-a-number");
    series->push_back(invalid_number);
    invalid_number->set_callback([&wait_group](WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        const void *body = nullptr;
        size_t body_len = 0;
        task->get_resp()->get_parsed_body(&body, &body_len);
        Json json = Json::parse(
            std::string(static_cast<const char *>(body), body_len));
        EXPECT_EQ(json["int"].get<int>(), 0);
        EXPECT_EQ(json["size"].get<size_t>(), 0U);
        EXPECT_DOUBLE_EQ(json["double"].get<double>(), 0.0);
        wait_group.done();
    });

    WFHttpTask *valid_number = create_http_task("/number/42");
    series->push_back(valid_number);
    valid_number->set_callback([&wait_group](WFHttpTask *task)
    {
        EXPECT_EQ(task->get_state(), WFT_STATE_SUCCESS);
        const void *body = nullptr;
        size_t body_len = 0;
        task->get_resp()->get_parsed_body(&body, &body_len);
        Json json = Json::parse(
            std::string(static_cast<const char *>(body), body_len));
        EXPECT_EQ(json["int"].get<int>(), 42);
        EXPECT_EQ(json["size"].get<size_t>(), 42U);
        EXPECT_DOUBLE_EQ(json["double"].get<double>(), 42.0);
        wait_group.done();
    });
    series->start();
    wait_group.wait();
    svr.stop();
}
