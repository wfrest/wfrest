#include "workflow/WFFacilities.h"
#include "workflow/MySQLResult.h"
#include <csignal>
#include "wfrest/HttpServer.h"
#include "wfrest/json.hpp"

using Json = nlohmann::json;
using namespace wfrest;
using namespace protocol;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main(int argc, char **argv)
{
    signal(SIGINT, sig_handler);

    HttpServer svr;
 
    svr.GET("/redis0", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Redis("redis://127.0.0.1/", "SET", {"test_val", "test_key"});
    });

    svr.GET("/redis1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Redis("redis://127.0.0.1/", "GET", {"test_val"});
    });

    if (svr.start(8888) == 0)
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

