#include "workflow/WFFacilities.h"
#include <csignal>
#include <gtest/gtest.h>
#include "wfrest/HttpServer.h"
#include "wfrest/BluePrint.h"

using namespace wfrest;

void set_admin_bp(BluePrint &bp)
{
    bp.GET("/page/new/", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("new page");
    });

    bp.GET("/page/edit", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("edit page");
    });
}

WFHttpTask *create_http_task(const std::string &path)
{
    return WFTaskFactory::create_http_task("http://127.0.0.1:8888/" + path, 4, 2, nullptr);
}

TEST(BluePrintTest, register_blueprint)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.POST("/page/{uri}", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "Blog Page\n");
    });

    BluePrint admin_bp;
    set_admin_bp(admin_bp);
    svr.register_blueprint(admin_bp, "/admin");

    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

    WFHttpTask *client_task_1 = create_http_task("admin/page/new");
    SeriesWork *series = Workflow::create_series_work(client_task_1, [&wait_group](const SeriesWork *) {
        wait_group.done();
    });
    client_task_1->set_callback([](WFHttpTask *task)
    {
        const void *body;
        size_t body_len;
        task->get_resp()->get_parsed_body(&body, &body_len);
        EXPECT_TRUE(strcmp("new page", static_cast<const char *>(body)) == 0);
    });
    WFHttpTask *client_task_2 = create_http_task("admin/page/new/");
    client_task_2->set_callback([](WFHttpTask *task)
    {
        const void *body;
        size_t body_len;
        task->get_resp()->get_parsed_body(&body, &body_len);
        EXPECT_TRUE(strcmp("new page", static_cast<const char *>(body)) == 0);
    });
    series->push_back(client_task_2);
    WFHttpTask *client_task_3 = create_http_task("admin/page/edit");
    client_task_3->set_callback([](WFHttpTask *task)
    {
        const void *body;
        size_t body_len;
        task->get_resp()->get_parsed_body(&body, &body_len);
        EXPECT_TRUE(strcmp("edit page", static_cast<const char *>(body)) == 0);
    });
    series->push_back(client_task_3);

    WFHttpTask *client_task_4 = create_http_task("admin/page/edit/");
    client_task_4->set_callback([](WFHttpTask *task)
    {
        const void *body;
        size_t body_len;
        task->get_resp()->get_parsed_body(&body, &body_len);
        EXPECT_TRUE(strcmp("edit page", static_cast<const char *>(body)) == 0);
    });
    series->push_back(client_task_4);

    series->start();
    wait_group.wait();
    svr.stop();
}
