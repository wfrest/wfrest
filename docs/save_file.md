## Save File

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // curl -v -X POST "ip:port/file_write1" -F "file=@filename" -H "Content-Type: multipart/form-data"
    svr.POST("/file_write1", [](const HttpReq *req, HttpResp *resp)
    {
        std::string& body = req->body();   // multipart/form - body has boundary
        resp->Save("test.txt", std::move(body));
    });

    svr.GET("/file_write2", [](const HttpReq *req, HttpResp *resp)
    {
        std::string content = "1234567890987654321";

        resp->Save("test1.txt", std::move(content));
    });

    // You can specify the message return to client when saving file successfully
    // default is "Save File success\n"
    svr.GET("/file_write3", [](const HttpReq *req, HttpResp *resp)
    {
        std::string content = "1234567890987654321";

        resp->Save("test2.txt", std::move(content), "test notify test successfully");
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
