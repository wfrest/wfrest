## Cookie

以下是设置和获取cookie的示例。

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // curl --cookie "name=chanchan, passwd=123" "http://ip:port/cookie"
    svr.GET("/cookie", [](const HttpReq *req, HttpResp *resp)
    {
        const std::map<std::string, std::string> &cookie = req->cookies();
        if(cookie.empty())  // 没有cookie
        {
            HttpCookie cookie;
            // 您可以设置的内容：
            // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie
            cookie.set_path("/").set_http_only(true);
            resp->add_cookie(std::move(cookie));
            resp->String("设置Cookie\n");
        }
        fprintf(stderr, "cookie :\n");
        for(auto &c : cookie)
        {
            fprintf(stderr, "%s : %s\n", c.first.c_str(), c.second.c_str());
        }
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

以下是一个更具体的示例，您可以查看[教程](https://github.com/wfrest/wfrest/discussions/60)

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    svr.GET("/login", [](const HttpReq *req, HttpResp *resp)
    {
        HttpCookie cookie;
        cookie.set_key("user")
                .set_value("chanchan")
                .set_path("/")
                .set_domain("localhost")
                .set_http_only(true);

        resp->add_cookie(std::move(cookie));
        resp->String("登录成功");
    });

    svr.GET("/home", [](const HttpReq *req, HttpResp *resp)
    {
        const std::string &cookie_val = req->cookie("user");
        if(cookie_val != "chanchan")
        {
            resp->set_status(HttpStatusUnauthorized);
            std::string err = R"(
            {
                "error": "未授权"
            }
            )";
            resp->Json(err);
        } else
        {
            std::string success = R"(
            {
                "home": "data"
            }
            )";
            resp->Json(success);
        }
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

## 如何向用户代理设置多个cookie？

参考：https://www.rfc-editor.org/rfc/rfc6265#section-5.4

如下一个例子所示，服务器可以在用户代理上存储多个cookie。
例如，服务器可以通过返回两个Set-Cookie头字段来存储会话标识符以及用户的首选语言。
请注意，服务器使用Secure和HttpOnly属性为更敏感的会话标识符提供额外的安全保护（参见第4.1.2节）。

== 服务器 -> 用户代理 ==

Set-Cookie: SID=31d4d96e407aad42; Path=/; Secure; HttpOnly
Set-Cookie: lang=en-US; Path=/; Domain=example.com

你可以这样编写代码：

```cpp
svr.GET("/multi", [](const HttpReq *req, HttpResp *resp)
{
    // 设置cookie
    HttpCookie cookie;

    cookie.set_key("user")
            .set_value("chanchan")
            .set_path("/")
            .set_domain("localhost")
            .set_http_only(true);

    resp->add_cookie(std::move(cookie));

    HttpCookie cookie2;
    cookie2.set_key("animal")
            .set_value("panda");
    resp->add_cookie(std::move(cookie2));

    resp->set_status(HttpStatusOK);
    resp->String("登录成功");
});
``` 