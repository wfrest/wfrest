#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

BluePrint file_server_logic()
{
    BluePrint bp;
    bp.GET("/text", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "Text File logic\n");
    });

    bp.GET("/images", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "images File logic\n");
    });
    return bp;
}

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;
    
    svr.POST("/login", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "Login Logic\n");
    });

    BluePrint bp = file_server_logic();

    svr.register_blueprint(bp, "/www/file");

    if (svr.start(8888) == 0)
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
