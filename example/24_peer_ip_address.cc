#include "workflow/WFFacilities.h"
#include <csignal>
#include <arpa/inet.h>
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

    // curl -v http://ip:port/hello
    svr.GET("/hello", [](const HttpReq *req, HttpResp *resp)
    {        
        auto *http_task = task_of(resp);

        fprintf(stderr, "Peer address: %s:%d\n", http_task->peer_addr().c_str(), http_task->peer_port());
        resp->String("world\n");
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