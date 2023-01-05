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

TEST(HttpServer, String_short_str)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.GET("/test", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("world\n");
    });
    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

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
    svr.stop();
}

TEST(HttpServer, String_short_str_gzip)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    auto *data = new std::string("Client send for test Gzip");

    svr.POST("/test", [data](const HttpReq *req, HttpResp *resp)
    {
        std::string &body = req->body();
        const std::string &compress_type = req->header("Content-Encoding");
        EXPECT_EQ(compress_type, "gzip");
        EXPECT_EQ(body, *data);
        resp->set_compress(Compress::GZIP);
        resp->String(std::move(body));
    });
    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

    WFHttpTask *client_task = create_http_task("test");

    auto *compress_data = new std::string;
    int ret = Compressor::gzip(data, compress_data);

    EXPECT_EQ(ret, StatusOK);
    HttpRequest *client_req = client_task->get_req();
    client_req->set_method("POST");
    client_req->add_header_pair("Content-Encoding", "gzip");
    client_req->append_output_body_nocopy(compress_data->data(), compress_data->size());

    client_task->set_callback([&wait_group, data, compress_data](WFHttpTask *task)
    {
        const void *body;
        size_t body_len;
        HttpResponse *resp = task->get_resp();
        resp->get_parsed_body(&body, &body_len);
        EXPECT_FALSE(strcmp(data->c_str(), static_cast<const char *>(body)) == 0);

        HttpHeaderMap header_map(resp);
        std::string compress_header = header_map.get("Content-Encoding");
        EXPECT_EQ(compress_header, "gzip");

        std::string decompress_data;
        int ret = Compressor::ungzip(static_cast<const char *>(body), body_len, &decompress_data);
        EXPECT_EQ(ret, StatusOK);
        EXPECT_EQ(decompress_data, *data);
        wait_group.done();
        delete data;
        delete compress_data;
    });

    client_task->start();
    wait_group.wait();
    svr.stop();
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
    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

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
    svr.stop();
}

TEST(HttpServer, multi_verb)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.ROUTE("/test", [](const HttpReq *req, HttpResp *resp)
    {
        std::string method(req->get_method());
        resp->String(std::move(method));
    }, {"GET", "POST"});

    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

    WFHttpTask *client_task_get = create_http_task("test");
    client_task_get->set_callback([](WFHttpTask *client_task)
    {
        const void *body;
        size_t body_len;

        client_task->get_resp()->get_parsed_body(&body, &body_len);
        EXPECT_TRUE(strcmp("GET", static_cast<const char *>(body)) == 0);
    });
    SeriesWork *series = Workflow::create_series_work(client_task_get,
                                [&wait_group](const SeriesWork *series) { wait_group.done(); });

    WFHttpTask *client_task_post = create_http_task("test");
    client_task_post->get_req()->set_method("POST");
    client_task_post->set_callback([](WFHttpTask *client_task)
    {
        const void *body;
        size_t body_len;

        client_task->get_resp()->get_parsed_body(&body, &body_len);
        EXPECT_TRUE(strcmp("POST", static_cast<const char *>(body)) == 0);
    });
    series->push_back(client_task_post);
    series->start();

    wait_group.wait();
    svr.stop();
}
