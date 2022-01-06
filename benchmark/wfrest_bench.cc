#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main(int argc, char** argv)
{
    signal(SIGINT, sig_handler);

    if (argc < 4) {
        // if threads eq 8, we recommend poller_threads = 2, handler_threads = 6
        printf("Usage: %s port [poller_threads] [handler_threads]\n", argv[0]);
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    struct WFGlobalSettings settings = GLOBAL_SETTINGS_DEFAULT;
    settings.poller_threads = atoi(argv[2]);
    settings.handler_threads = atoi(argv[3]);
    WORKFLOW_library_init(&settings);

    HttpServer svr;

    // wrk -t100 -c1000 -d30s  --latency http://ip:port/
    svr.GET("/", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("");
    });

    // wrk -t100 -c1000 -d30s  --latency http://ip:port/ping
    svr.GET("/ping", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("pong");
    });

    // wrk -t100 -c1000 -d30s -s post.lua --latency http://ip:port/echo
    svr.POST("/echo", [](const HttpReq *req, HttpResp *resp)
    {
        std::string &body = req->body();
        resp->String(std::move(body));
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