#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"

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

    // curl -v http://ip:port/gzip
    svr.GET("/gzip", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("Test for sending gzip data\n", Compress::GZIP);
    });

    if (svr.start(8888) == 0)
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
