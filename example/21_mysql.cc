#include "workflow/WFFacilities.h"
#include "workflow/MySQLResult.h"
#include <csignal>
#include "wfrest/HttpServer.h"

using namespace wfrest;
using namespace protocol;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main(int argc, char **argv)
{
    signal(SIGINT, sig_handler);

    HttpServer svr;
 
    svr.GET("/mysql0", [](const HttpReq *req, HttpResp *resp)
    {
        resp->MySQL("mysql://root:111111@localhost", "SHOW DATABASES");
    });

    svr.GET("/mysql1", [](const HttpReq *req, HttpResp *resp)
    {
        std::string url = "mysql://root:111111@localhost";
        resp->MySQL(url, "SHOW DATABASES", [resp](wfrest::Json *json) 
        {
            wfrest::Json js;
            js["rows"] = (*json)["result_set"][0]["rows"];
            resp->String(js.dump());
        });
    });

    svr.GET("/mysql2", [](const HttpReq *req, HttpResp *resp)
    {
        std::string url = "mysql://root:111111@localhost";

        resp->MySQL(url, "SHOW DATABASES", [resp](MySQLResultCursor *cursor) 
        {
            std::string res;
            std::vector<MySQLCell> arr;
            while (cursor->fetch_row(arr))
            {
                res.append(arr[0].as_string());
                res.append("\n");
            }
            resp->String(std::move(res));
        });

    });

    svr.GET("/multi", [](const HttpReq *req, HttpResp *resp)
    {
        resp->MySQL("mysql://root:111111@localhost/wfrest_test", "SHOW DATABASES; SELECT * FROM wfrest");
    });

    svr.POST("/client", [](const HttpReq *req, HttpResp *resp)
    {
        std::string &sql = req->body();
        resp->MySQL("mysql://root:111111@localhost/wfrest_test", std::move(sql));
    });

    if (svr.start(8888) == 0)
    {
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server\n");
        exit(1);
    }
    return 0;
}

