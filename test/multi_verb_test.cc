#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    svr.PUT("/account", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "put account\n");
        resp->String("put verb");
    });

    svr.DELETE("/account", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "delete account\n");
        resp->String("delete verb");
    });

    svr.POST("/account", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "post account\n");
        resp->String("post verb");
    });

    if (svr.track().start(8888) == 0)
    {
        svr.list_routes();
        getchar();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}