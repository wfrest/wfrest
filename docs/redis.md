## Redis

```cpp
#include "wfrest/HttpServer.h"

using namespace wfrest;

int main(int argc, char **argv)
{
    HttpServer svr;

    svr.GET("/redis0", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Redis("redis://127.0.0.1/", "SET", {"test_val", "test_key"});
    });

    svr.GET("/redis1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Redis("redis://127.0.0.1/", "GET", {"test_val"});
    });

    if (svr.start(8888) == 0)
    {
        getchar();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server\n");
        exit(1);
    }
    return 0;
}
```

default return format is json.

## Format of Redis URL

```
redis://:password@host:port/dbnum?query#fragment
```

If SSL is used, use:

```
rediss://:password@host:port/dbnum?query#fragment
```

password is optional. The default port is 6379; the default dbnum is 0, and its range is from 0 to 15.
query and fragment are not used in the factory and you can define them by yourself. For example, if you want to use upstream
selection , you can define your own query and fragment. For relevant details, please see upstream documents.
Sample Redis URL:

```
redis://127.0.0.1/

redis://:12345678@redis.some-host.com/1
```


