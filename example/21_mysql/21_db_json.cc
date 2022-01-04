#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main(int argc, char **argv)
{
    signal(SIGINT, sig_handler);

    HttpServer svr;

    svr.GET("/show_db", [](const HttpReq *req, HttpResp *resp, SeriesWork *series)
    {
        resp->MySQL("mysql://root:111111@localhost", "SHOW DATABASES");
    });

    svr.GET("/query", [](const HttpReq *req, HttpResp *resp, SeriesWork *series)
    {
        resp->MySQL("mysql://root:111111@localhost/wfrest_test", "SELECT * FROM wfrest");
    });

    svr.GET("/multi", [](const HttpReq *req, HttpResp *resp, SeriesWork *series)
    {
        resp->MySQL("mysql://root:111111@localhost/wfrest_test", "SHOW DATABASES; SELECT * FROM wfrest");
    });

    if (svr.start(8888, argv[1], argv[2]) == 0)
    {
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server\n");
        exit(1);
    }
    return 0;
}

