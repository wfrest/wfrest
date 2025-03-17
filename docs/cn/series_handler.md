## 系列处理器

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    svr.GET("/series", [](const HttpReq *req, HttpResp *resp, SeriesWork* series)
    {
        auto *timer = WFTaskFactory::create_timer_task(5000000, [](WFTimerTask *) {
            printf("定时器任务完成(5秒)。\n");
        });

        series->push_back(timer);
    });

    if (svr.start(8888) == 0)
    {
        getchar();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
``` 