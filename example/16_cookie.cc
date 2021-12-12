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
