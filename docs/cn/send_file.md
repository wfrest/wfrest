## 发送文件

发送文件的操作十分方便，只需要调用`resp->File`, 还可以加上range参数发送部分文件。

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // single files
    svr.GET("/file1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt");
    });

    svr.GET("/file2", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("html/index.html");
    });

    svr.GET("/file3", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("/html/index.html");
    });

    svr.GET("/file4", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 0);
    });

    svr.GET("/file5", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 0, 10);
    });

    svr.GET("/file6", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 5, 10);
    });

    svr.GET("/file7", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 5, -1);
    });

    svr.GET("/file8", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", -5, -1);
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
