//
// Created by Chanchan on 11/4/21.
//

#include "HttpServer.h"
#include "HttpMsg.h"
#include "workflow/WFFacilities.h"
#include "workflow/HttpUtil.h"
#include <csignal>
#include "json.hpp"

using namespace wfrest;

using json = nlohmann::json;

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
    svr.Get("/user/{name}", [](HttpReq *req, HttpResp *resp)
    {
        std::string name = req->param("name");
        // resp->set_status(HttpStatusOK); // automatically
        resp->String("Hello " + name + "\n");
    });

    // However, this one will match
    // wildcast/chanchan/action... (prefix)
    svr.Get("/wildcast/{name}/action*", [](HttpReq *req, HttpResp *resp)
    {
        std::string name = req->param("name");
        std::string message = name + " : path " + req->get_request_uri();

        resp->String("Hello " + message + "\n");
    });

    if (svr.start(9001) == 0)
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