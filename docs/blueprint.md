## BluePrint

wfrest supports flask style blueprints. 

You can see [What are Flask Blueprints, exactly?](https://stackoverflow.com/questions/24420857/what-are-flask-blueprints-exactly)

A blueprint is a limited wfresr server. It cannot handle networking. But it can handle routes.

For bigger projects, all your code shouldn't be in the same file. Instead you can segment or split bigger codes into separate files which makes your project a lot more modular.

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

// You can split your different business logic into different files / modules
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