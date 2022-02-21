## 代理服务器

直接利用`resp->Http()`完成转发

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    svr.GET("/proxy", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Http("http://www.baidu.com");
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