## Send File

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

    // multiple files
    svr.GET("/multi_files", [](const HttpReq *req, HttpResp *resp)
    {
        std::vector<std::string> file_list = {"test.txt", "todo.txt", "test1.txt"};
        resp->File(file_list);
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
