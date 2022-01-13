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
    
    svr.GET("/config", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("config");
    });

    if (svr.max_connections(4000)
            .peer_response_timeout(20 * 1000)
            .keep_alive_timeout(30 * 1000)
            .track()
            .start(8888) == 0)
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

