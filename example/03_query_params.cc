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

    // The request responds to a url matching:  /query_list?username=chanchann&password=yyy
    svr.GET("/query_list", [](const HttpReq *req, HttpResp *resp)
    {
        const std::map<std::string, std::string>& query_list = req->query_list();
        for (auto &query: query_list)
        {
            fprintf(stderr, "%s : %s\n", query.first.c_str(), query.second.c_str());
        }
    });

    // The request responds to a url matching:  /query?username=chanchann&password=yyy
    svr.GET("/query", [](const HttpReq *req, HttpResp *resp)
    {
        const std::string& user_name = req->query("username");
        const std::string& password = req->query("password");
        const std::string& info = req->query("info"); // no this field
        const std::string& address = req->default_query("address", "china");
        resp->String(user_name + " " + password + " " + info + " " + address + "\n");
    });

    // The request responds to a url matching:  /query_has?username=chanchann&password=
    // The logic for judging whether a parameter exists is that if the parameter value is empty, the parameter is considered to exist
    // and the parameter does not exist unless the parameter is submitted.
    svr.GET("/query_has", [](const HttpReq *req, HttpResp *resp)
    {
        if (req->has_query("password"))
        {
            fprintf(stderr, "has password query\n");
        }
        if (req->has_query("info"))
        {
            fprintf(stderr, "has info query\n");
        }
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
