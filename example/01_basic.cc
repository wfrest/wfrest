#include "workflow/WFFacilities.h"
#include "workflow/HttpUtil.h"
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

    // curl -v http://ip:port/hello
    svr.Get("/hello", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("world\n");
    });
    // curl -v http://ip:port/data
    svr.Get("/data", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("Hello world\n", 12);
    });

    // curl -v http://ip:port/post -d 'post hello world'
    svr.Post("/post", [](const HttpReq *req, HttpResp *resp)
    {
        std::string body = req->Body();
        fprintf(stderr, "post data : %s\n", body.c_str());
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