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

    svr.GET("/login", [](const HttpReq *req, HttpResp *resp)
    {
        // set cookie
        HttpCookie cookie;

        cookie.set_key("user")
                .set_value("chanchan")
                .set_path("/")
                .set_domain("localhost")
                .set_http_only(true);

        resp->add_cookie(std::move(cookie));
        resp->set_status(HttpStatusOK);
        resp->String("Login success");
    });

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
            resp->set_status(HttpStatusOK);
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
        svr.list_routes();
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
