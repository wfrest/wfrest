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


    svr.GET("/v1/v2", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("world\n");
    });

    svr.GET("/v1/v2/{uuid}", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("world\n");
    });

    if (svr.track().start(8888) == 0)
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