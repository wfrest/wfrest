## Timer(sleep)

Timers are used to specify a certain waiting time without occupying a thread.

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    svr.GET("/timer1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Timer(5, 0);  // seconds, nanoseconds
        resp->String("Test timer");
    });

    svr.GET("/timer2", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Timer(5000 * 5000);  // microseconds
        resp->String("Test timer");
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