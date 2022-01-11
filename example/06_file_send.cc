#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;

    // single files
    svr.GET("/file1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt");
    });

    svr.GET("/file2", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("html/index.html");
    });

    svr.GET("/file3", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("/html/index.html");
    });

    svr.GET("/file4", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 0);
    });

    svr.GET("/file5", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 0, 10);
    });

    svr.GET("/file6", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 5, 10);
    });

    svr.GET("/file7", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 5, -1);
    });

    svr.GET("/file8", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", -5, -1);
    });

    if (svr.start(8888) == 0)
    {
        svr.list_routes();
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
