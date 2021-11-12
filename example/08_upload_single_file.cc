//
// Created by Chanchan on 11/9/21.
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

    // curl -v -X POST "ip:port/file_write1" -F "file=@filename" -H "Content-Type: multipart/form-data"
    svr.Post("/file_write1", [](const HttpReq *req, HttpResp *resp)
    {
        auto *ctx = new write_ctx;
        ctx->content = req->Body();   // multipart/form - body has boundary
        auto *server_task = req->get_task();
        server_task->user_data = ctx;    // for delete in server_task's callback

        resp->Save("test.txt", ctx->content);

        server_task->set_callback([](const WebTask *server_task) {
            delete static_cast<write_ctx *>(server_task->user_data);
        });
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
