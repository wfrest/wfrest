## 蓝图（BluePrint）

wfrest支持Flask风格的蓝图。 

您可以参考 [Flask蓝图到底是什么？](https://stackoverflow.com/questions/24420857/what-are-flask-blueprints-exactly)

蓝图是一个有限制的wfrest服务器。它不能处理网络通信，但可以处理路由。

对于更大的项目，您的所有代码不应该在同一个文件中。相反，您可以将较大的代码分割或拆分到不同的文件中，这使您的项目更具模块化。

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

// 您可以将不同的业务逻辑拆分到不同的文件/模块中
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