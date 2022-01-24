#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

void Factorial(int n, HttpResp *resp)
{
    unsigned long long factorial = 1;
    for(int i = 1; i <= n; i++)
    {
        factorial *= i;
    }
    resp->String("fac(" + std::to_string(n) + ") = " + std::to_string(factorial));
}

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;

    svr.GET("/compute", [](const HttpReq *req, HttpResp *resp)
    {
        // First param is queue id
        resp->Compute(1, Factorial, 10, resp);
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