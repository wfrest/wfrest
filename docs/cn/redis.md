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

默认返回格式为json。

## Redis URL格式

```
redis://:password@host:port/dbnum?query#fragment
```

如果使用SSL，请使用：

```
rediss://:password@host:port/dbnum?query#fragment
```

密码是可选的。默认端口是6379；默认dbnum是0，其范围是0到15。
query和fragment在工厂中不使用，您可以自己定义它们。例如，如果您想使用上游选择，可以定义自己的query和fragment。有关详细信息，请参阅上游文档。
Redis URL示例：

```
redis://127.0.0.1/

redis://:12345678@redis.some-host.com/1
``` 