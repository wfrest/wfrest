#include "workflow/WFFacilities.h"
#include <csignal>
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

    svr.GET("/series", [](const HttpReq *req, HttpResp *resp, SeriesWork* series)
    {
        auto *timer = WFTaskFactory::create_timer_task(5000000, [](WFTimerTask *) {
            printf("timer task complete(5s).\n");
        });

        series->push_back(timer);
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
