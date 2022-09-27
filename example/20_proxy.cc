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

    // curl -v http://ip:port/proxy
    svr.GET("/proxy", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Http("http://www.baidu.com");
    });

    svr.GET("/bing", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Http("www.bing.com");
    });

    svr.GET("/param", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Http("127.0.0.1:8000/path?a=1");
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