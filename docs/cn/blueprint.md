## 蓝图

wfrest支持像flask一样的蓝图。

可以看[如何理解Flask中的蓝本/图?](https://www.zhihu.com/question/31748237)

一个蓝图是有限功能的wfrest服务器，他不提供网络交互的功能，但是他能注册路由。

对于一个稍微大型的项目来说，你的所有代码最好不要放到一个文件里，应该考虑模块化。

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

void set_admin_bp(BluePrint &bp)
{
    bp.GET("/page/new/", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "New page\n");
    });

    bp.GET("/page/edit/", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "Edit page\n");
    });
}

int main()
{
    HttpServer svr;
    
    svr.POST("/page/{uri}", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "Blog Page\n");
    });

    BluePrint admin_bp;
    set_admin_bp(admin_bp);

    svr.register_blueprint(admin_bp, "/admin");

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