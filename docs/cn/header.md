## 请求头

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    svr.POST("/post", [](const HttpReq *req, HttpResp *resp)
    {
        const std::string& host = req->header("Host");
        const std::string& content_type = req->header("Content-Type");
        if (req->has_header("User-Agent"))
        {
            fprintf(stderr, "存在User-Agent...");
        }
        resp->String(host + " " + content_type + "\n");
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