//
// Created by Chanchan on 11/9/21.
//

#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"
#include "HttpMsg.h"

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

    // curl -v -X POST "ip:port/file_write1" -F "file=@filename" -H "Content-Type: multipart/form-data"
    svr.Post("/file_write1", [](const HttpReq *req, HttpResp *resp)
    {
        std::string body = req->Body();   // multipart/form - body has boundary
        resp->Save("test.txt", std::move(body));
    });

    svr.Get("/file_write2", [](const HttpReq *req, HttpResp *resp)
    {
        std::string content = "1234567890987654321";

        resp->Save("test1.txt", std::move(content));
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
