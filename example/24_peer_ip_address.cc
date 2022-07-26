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

    // curl -v http://ip:port/method1
    svr.GET("/method1", [](const HttpReq *req, HttpResp *resp)
    {        
        char addrstr[128];
        struct sockaddr_storage addr;
        socklen_t l = sizeof addr;
        unsigned short port = 0;

        task_of(resp)->get_peer_addr((struct sockaddr *)&addr, &l);
        if (addr.ss_family == AF_INET)
        {
            struct sockaddr_in *sin = (struct sockaddr_in *)&addr;
            inet_ntop(AF_INET, &sin->sin_addr, addrstr, 128);
            port = ntohs(sin->sin_port);
        }
        else if (addr.ss_family == AF_INET6)
        {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addr;
            inet_ntop(AF_INET6, &sin6->sin6_addr, addrstr, 128);
            port = ntohs(sin6->sin6_port);
        }
        else
            strcpy(addrstr, "Unknown");

        fprintf(stderr, "Peer address: %s:%d\n", addrstr, port);
        resp->String("world\n");
    });

    // An easier way
    // curl -v http://ip:port/method2
    svr.GET("/method2", [](const HttpReq *req, HttpResp *resp)
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