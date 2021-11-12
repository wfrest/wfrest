//
// Created by Chanchan on 11/8/21.
//


#include "HttpServer.h"
#include "HttpMsg.h"
#include "workflow/WFFacilities.h"
#include <csignal>

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
    svr.mount("static");

    svr.Get("/file1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt");
    });

    svr.Get("/file2", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("html/index.html");
    });

    svr.Get("/file3", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("/html/index.html");
    });

    svr.Get("/file4", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 0);
    });

    svr.Get("/file5", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 0, 10);
    });

    svr.Get("/file6", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 5, 10);
    });

    if (svr.start(9001) == 0)
    {
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
