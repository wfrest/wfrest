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

    // This handler will match /user/chanchan but will not match /user/ or /user
    // curl -v "ip:port/user/chanchan/"
    svr.GET("/user/{name}", [](const HttpReq *req, HttpResp *resp)
    {
        // reference : no copy
        const std::string& name = req->param("name");
        // resp->set_status(HttpStatusOK); // automatically
        resp->String("Hello " + name + "\n");
    });

    // wildcast/chanchan/action... (prefix)
    svr.GET("/wildcast/{name}/action*", [](const HttpReq *req, HttpResp *resp)
    {
        const std::string& name = req->param("name");
        const std::string& match_path = req->match_path();

        resp->String("[name : " + name + "] [match path : " + match_path + "]\n");
    });

    // request will hold the route definition
    svr.GET("/user/{name}/match*", [](const HttpReq *req, HttpResp *resp)
    {
        // if /user/chanchan/match1234
        // full_path : /user/{name}/match*
        // current_path : /user/chanchan/match1234
        // match_path: match1234
        const std::string &full_path = req->full_path();
        const std::string &current_path = req->current_path();
        std::string res;
        if (full_path == "/user/{name}/match*")
        {
            res = full_path + " match : " + current_path;
        } else
        {
            res = full_path + " dosen't match";
        }
        resp->String(res);
    });

    svr.GET("/public/*", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "full_path : %s\n", req->full_path().c_str());
        fprintf(stderr, "current_path : %s\n", req->current_path().c_str());
        fprintf(stderr, "current_path : %s\n", req->match_path().c_str());
    });

    // This handler will add a new router for /user/groups.
    // Exact routes are resolved before param routes, regardless of the order they were defined.
    // Routes starting with /user/groups are never interpreted as /user/{name}/... routes
    svr.GET("/user/groups", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String(req->full_path());
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