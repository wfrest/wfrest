#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

void Fibonacci(int n, HttpResp *resp)
{
    unsigned long long x = 0, y = 1;
    char buf[256];
    int i;

    if (n <= 0 || n > 94)
    {
        resp->append_output_body_nocopy("<html>Invalid Number.</html>",
                                        strlen("<html>Invalid Number.</html>"));
        return;
    }

    resp->append_output_body_nocopy("<html>", strlen("<html>"));
    for (i = 2; i < n; i++)
    {
        sprintf(buf, "<p>%llu + %llu = %llu.</p>", x, y, x + y);
        resp->append_output_body(buf);
        y = x + y;
        x = y - x;
    }

    if (n == 1)
        y = 0;
    sprintf(buf, "<p>The No. %d Fibonacci number is: %llu.</p>", n, y);
    resp->append_output_body(buf);
    resp->append_output_body_nocopy("</html>", strlen("</html>"));
}

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;

    svr.GET("/compute_task", 1,[](const HttpReq *req, HttpResp *resp)
    {
        Fibonacci(20, resp);
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
