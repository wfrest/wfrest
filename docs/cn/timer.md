## 定时器(休眠)

定时器用于指定一定的等待时间，而不占用线程。

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    svr.GET("/timer1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Timer(5, 0, [resp] { resp->String("测试定时器1"); });  // 秒，纳秒=
    });

    svr.GET("/timer2", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Timer(5000 * 1000, [resp] { resp->String("测试定时器2"); });  // 微秒
    });

    if (svr.track().start(8888) == 0)
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