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

## Range semantics

`File(path, start, end)` and `CachedFile(path, start, end)` use a half-open
range: `start` is included and `end` is excluded. Passing `-1` as `end` reads
through EOF. A negative `start` selects a suffix, so `File(path, -5, -1)`
returns the final five bytes.

Full-file responses use status `200` without a `Content-Range` header. Partial
responses use status `206` and an RFC 9110 `Content-Range` header whose final
byte position is inclusive. Ends beyond EOF are clamped. Invalid or empty
partial ranges return status `416` with `Content-Range: bytes */<file-size>`.
