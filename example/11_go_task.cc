#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"
#include "GoTaskWrapper.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

void foo()
{
    fprintf(stderr,"function pointer\n");
}

void func(HttpResp* resp)
{
    resp->String("foo resp");
}

struct A {
    void fA() { fprintf(stderr, "std::bind\n"); }
    void fB() { fprintf(stderr, "std::function\n"); }
};

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;

//    svr.GET("go", [](const HttpReq *req, HttpResp *resp, SeriesWork *series)

    svr.GET("/go_foo", [](const HttpReq *req, HttpResp *resp)
    {
        go foo;
    });

    svr.GET("/go_bind", [](const HttpReq *req, HttpResp *resp)
    {
        go std::bind(&A::fA, A());
    });

    svr.GET("/go_functional", [](const HttpReq *req, HttpResp *resp)
    {
        std::function<void()> fn(std::bind(&A::fB, A()));
        go fn;
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
