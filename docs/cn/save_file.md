## 保存文件

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // curl -v -X POST "ip:port/file_write1" -F "file=@filename" -H "Content-Type: multipart/form-data"
    svr.POST("/file_write1", [](const HttpReq *req, HttpResp *resp)
    {
        std::string& body = req->body();   // multipart/form - body有边界
        resp->Save("test.txt", std::move(body));
    });

    svr.GET("/file_write2", [](const HttpReq *req, HttpResp *resp)
    {
        std::string content = "1234567890987654321";

        resp->Save("test1.txt", std::move(content));
    });

    // 您可以指定保存文件成功时返回给客户端的消息
    // 默认是 "Save File success\n"
    svr.GET("/file_write3", [](const HttpReq *req, HttpResp *resp)
    {
        std::string content = "1234567890987654321";

        resp->Save("test2.txt", std::move(content), "测试通知测试成功");
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