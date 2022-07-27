#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"
#include "wfrest/CodeUtil.h"

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

    // curl -v http://ip:port/你好
    // or you can use 25_cn_url_client
    svr.GET(CodeUtil::url_encode("/你好"), [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("world\n");
    });

    svr.GET(CodeUtil::url_encode("/你好/吃了吗/"), [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("eat\n");
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