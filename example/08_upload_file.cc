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
    svr.mount("/static");

    // curl -v -X POST "ip:port/upload" -F "file=@filename" -H "Content-Type: multipart/form-data"
    svr.Post("/upload", [](HttpReq *req, HttpResp *resp)
    {
        auto files = req->post_files();
        if(files.empty())
        {
            resp->set_status(HttpStatusBadRequest);
        } else
        {
            auto *file = files[0];
            resp->Save(file->filename, std::move(file->content));
        }
    });

    // upload multiple files
    // curl -X POST http://ip:port/upload_multiple \
    // -F "file1=@file1" \
    // -F "file2=@file2" \
    // -H "Content-Type: multipart/form-data"
    svr.Post("/upload_multiple", [](HttpReq *req, HttpResp *resp)
    {
        auto files = req->post_files();
        if(files.empty())
        {
            resp->set_status(HttpStatusBadRequest);
        } else
        {
            for(auto& file : files)
            {
                resp->Save(file->filename, std::move(file->content));
            }
        }
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
