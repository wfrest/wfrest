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

    svr.GET("/timer1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Timer(5, 0, [resp] { resp->String("Test tiemr1"); });  // seconds, nanoseconds=
    });

    svr.GET("/timer2", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Timer(5000 * 1000, [resp] { resp->String("Test tiemr2"); });  // microseconds
    });

    svr.GET("/notimer", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("No timer");
    });

    if (svr.track().start(8888) == 0)
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