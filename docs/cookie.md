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