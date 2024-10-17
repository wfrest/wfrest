## Build-in json (wfrest::Json) usage

We provide built-in json, and it's strongly recommended.

If you want to get more json api usage, you can see [json api](json_api.md)

```cpp
#include "wfrest/HttpServer.h"
#include "wfrest/Json.h"

using namespace wfrest;

int main()
{
    HttpServer svr;

    // curl -v http://ip:port/json1
    svr.GET("/json1", [](const HttpReq *req, HttpResp *resp)
    {
        Json json;
        json["test"] = 123;
        json["json"] = "test json";
        resp->Json(json);
    });

    // recieve json
    //   curl -X POST http://ip:port/json2
    //   -H 'Content-Type: application/json'
    //   -d '{"login":"my_login","password":"my_password"}'
    svr.POST("/json2", [](const HttpReq *req, HttpResp *resp)
    {
        if (req->content_type() != APPLICATION_JSON)
        {
            resp->String("NOT APPLICATION_JSON");
            return;
        }
        fprintf(stderr, "Json : %s", req->json().dump(4).c_str());
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

