## 重定向

我们可以使用重定向实现页面之间的导航，这是通过Location头实现的。相关方法：

```cpp
void Redirect(const std::string& location, int status_code);
```

重定向用于引导客户端导航到指定地址，可以是本地服务的相对路径，也可以是完整的HTTP地址。使用示例：

```cpp
#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;

    // curl -L 127.0.0.1:8888/redirect
    svr.GET("/redirect", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Redirect("/test", HttpStatusMovedPermanently);
    });

    svr.GET("/test", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("{hello : world}");
    });

    if (svr.track().start(8888) == 0)
    {
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
```

运行后，我们可以通过浏览器访问http://127.0.0.1:8888/redirect，然后发现浏览器立即重定向到http://127.0.0.1:8888/test页面。 