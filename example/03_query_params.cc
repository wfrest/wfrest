//
// Created by Chanchan on 11/4/21.
//

#include "HttpServer.h"
#include "HttpMsg.h"
#include "workflow/WFFacilities.h"
#include <csignal>

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

    // The request responds to a url matching:  /query_list?username=chanchann&password=yyy
    svr.Get("/query_list", [](HttpReq *req, HttpResp *resp)
    {
        auto query_list = req->query_list();
        for(auto& query : query_list)
        {
            fprintf(stderr, "%s : %s\n", query.first.c_str(), query.second.c_str());
        }
    });

    // The request responds to a url matching:  /query?username=chanchann&password=yyy
    svr.Get("/query", [](HttpReq *req, HttpResp *resp)
    {
        std::string user_name = req->query("username");
        std::string password = req->query("password");
        std::string info = req->query("info"); // no this field
        std::string address = req->default_query("address", "china");
        resp->String(user_name + " " + password + " " + info + " " + address + "\n");
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
