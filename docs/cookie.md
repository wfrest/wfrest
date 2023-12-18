## Cookie

Here is a example for set and get a cookie.

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
        if(cookie.empty())  // no cookie
        {
            HttpCookie cookie;
            // What you can set :
            // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie
            cookie.set_path("/").set_http_only(true);
            resp->add_cookie(std::move(cookie));
            resp->String("Set Cookie\n");
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

Here is a more specific example, you can see the the [tutorial](https://github.com/wfrest/wfrest/discussions/60)

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
        resp->String("Login success");
    });

    svr.GET("/home", [](const HttpReq *req, HttpResp *resp)
    {
        const std::string &cookie_val = req->cookie("user");
        if(cookie_val != "chanchan")
        {
            resp->set_status(HttpStatusUnauthorized);
            std::string err = R"(
            {
                "error": "Unauthorized"
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

## How to set multiple cookies to user agent?

ref : https://www.rfc-editor.org/rfc/rfc6265#section-5.4

As shown in the next example, the server can store multiple cookies
at the user agent.  For example, the server can store a session
identifier as well as the user's preferred language by returning two
Set-Cookie header fields.  Notice that the server uses the Secure and
HttpOnly attributes to provide additional security protections for
the more sensitive session identifier (see Section 4.1.2.)

== Server -> User Agent ==

Set-Cookie: SID=31d4d96e407aad42; Path=/; Secure; HttpOnly
Set-Cookie: lang=en-US; Path=/; Domain=example.com

You can write code like this:

```cpp
svr.GET("/multi", [](const HttpReq *req, HttpResp *resp)
{
    // set cookie
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
    resp->String("Login success");
});
```
