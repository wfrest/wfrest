# ✨ wfrest: C++ Web Framework REST API

wfrest 是一个 快速🚀, 高效⌛️, 最为简单易用的💥 c++ 异步web框架.

wfrest 基于 [✨**C++ Workflow**✨](https://github.com/sogou/workflow).

[**C++ Workflow**](https://github.com/sogou/workflow) 是一个设计轻盈优雅的企业级程序引擎.

你可以用来：

- 快速搭建http服务器：

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    svr.GET("/hello", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("world\n");
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

# 使用中有疑问？

可以先查看[Discussions](https://github.com/wfrest/wfrest/discussions)和[issues](https://github.com/wfrest/wfrest/issues)列表，看看是否能找到答案。

非常欢迎将您使用中遇到的问题发送到issues。

也可以通过QQ群：884394197 联系我们。