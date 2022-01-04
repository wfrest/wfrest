#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"
#include "wfrest/Mysql.h"
#include "wfrest/MysqlUtil.h"

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

    svr.GET("/db", [](const HttpReq *req, HttpResp *resp, SeriesWork *series)
    {
        MySQL db("mysql://root:111111@localhost");

        db.query("", [](MySQLResultCursor &cursor, 
                            const MySQL::Status &status)
        {
            fprintf(stderr, "query 1\n");
        });

        db.query("", [](MySQLResultCursor &cursor, 
                            const MySQL::Status &status)
        {
            fprintf(stderr, "query 2\n");
        });

        db.query("", [](MySQLResultCursor &cursor, 
                            const MySQL::Status &status)
        {
            fprintf(stderr, "query 3\n");
        });
        *series << db;
    });

    svr.GET("/show_db", [](const HttpReq *req, HttpResp *resp, SeriesWork *series)
    {
        MySQL db("mysql://root:111111@localhost");
        db.query("SHOW DATABASES", [resp](MySQLResultCursor &cursor, 
                                        const MySQL::Status &status)
        {
            std::string res;
            std::vector<MySQLCell> arr;
            while (cursor.fetch_row(arr))
            {
                res.append(arr[0].as_string());
                res.append("\n");
            }
            resp->String(std::move(res));
        });
        *series << db;
    });

    if (svr.start(8888, argv[1], argv[2]) == 0)
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
