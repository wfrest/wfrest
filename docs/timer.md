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
        resp->Timer(5, 0, [resp] { resp->String("Test tiemr1"); });  // seconds, nanoseconds=
    });

    svr.GET("/timer2", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Timer(5000 * 1000, [resp] { resp->String("Test tiemr2"); });  // microseconds
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