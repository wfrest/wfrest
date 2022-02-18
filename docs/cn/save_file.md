## 保存文件

保存文件只需要调用`resp->Save`即可异步写文件到服务器。

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    svr.POST("/file_write1", [](const HttpReq *req, HttpResp *resp)
    {
        std::string& body = req->body();   
        resp->Save("test.txt", std::move(body));
    });

    svr.GET("/file_write2", [](const HttpReq *req, HttpResp *resp)
    {
        std::string content = "1234567890987654321";

        resp->Save("test1.txt", std::move(content));
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
