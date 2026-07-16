## 发送文件

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // 单个文件
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

## 范围语义

`File(path, start, end)` 和 `CachedFile(path, start, end)` 使用左闭右开区间：
包含 `start`，不包含 `end`。将 `end` 设为 `-1` 表示读取到文件末尾；负数
`start` 表示从文件尾部倒数，例如 `File(path, -5, -1)` 返回最后 5 个字节。

完整文件响应使用 `200`，不包含 `Content-Range`。部分文件响应使用 `206`，
并按照 RFC 9110 生成末字节位置为闭区间的 `Content-Range`。超过文件末尾的
`end` 会被截断；无效范围或空的部分范围返回 `416`，同时包含
`Content-Range: bytes */<file-size>`。
