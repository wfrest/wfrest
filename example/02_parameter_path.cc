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
    svr.GET("/user/{name}", [](HttpReq *req, HttpResp *resp)
    {
        std::string name = req->param("name");
        // resp->set_status(HttpStatusOK); // automatically
        resp->String("Hello " + name + "\n");
    });

    // wildcast/chanchan/action... (prefix)
    svr.GET("/wildcast/{name}/action*", [](HttpReq *req, HttpResp *resp)
    {
        std::string name = req->param("name");
        std::string message = name + " : path " + req->get_request_uri();

        resp->String("Hello " + message + "\n");
    });

    // request will hold the route definition
    svr.GET("/user/{name}/match*", [](HttpReq *req, HttpResp *resp)
    {
        std::string full_path = req->full_path();
        if (full_path == "/user/{name}/match*")
        {
            full_path += " match";
        } else
        {
            full_path += " dosen't match";
        }
        resp->String(full_path);
    });

    // This handler will add a new router for /user/groups.
    // Exact routes are resolved before param routes, regardless of the order they were defined.
    // Routes starting with /user/groups are never interpreted as /user/{name}/... routes
    svr.GET("/user/groups", [](HttpReq *req, HttpResp *resp)
    {
        resp->String(req->full_path());
    });

    if (svr.start(8888) == 0)
    {
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}