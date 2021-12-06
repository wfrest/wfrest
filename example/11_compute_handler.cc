#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

void Fibonacci(int n, HttpResp *resp)
{
    unsigned long long x = 0, y = 1;
    if (n <= 0 || n > 94)
    {
        fprintf(stderr, "invalid parameter");
        return;
    }
    for (int i = 2; i < n; i++)
    {
        y = x + y;
        x = y - x;
    }
    if (n == 1)
        y = 0;
    resp->String("fib(" + std::to_string(n) + ") is : " + std::to_string(y) + "\n");
}

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;

    // Second parameter means this computing queue id is 1
    // This handler is for computing
    // curl -v http://ip:port/compute_task?num=20
    svr.GET("/compute_task", 1, [](const HttpReq *req, HttpResp *resp)
    {
        int num = std::stoi(req->query("num"));
        Fibonacci(num, resp);
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
