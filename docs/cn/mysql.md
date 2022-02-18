## MySQL

MySQL接口目前支持多种模式

基础模式是暴露json接口，可以选择自己剪枝。

还有cursor接口，更加高效操作MySQL返回的结果。

```cpp
#include "workflow/MySQLResult.h"
#include "wfrest/HttpServer.h"
#include "wfrest/json.hpp"

using Json = nlohmann::json;
using namespace wfrest;
using namespace protocol;

int main(int argc, char **argv)
{
    HttpServer svr;
 
    svr.GET("/mysql00", [](const HttpReq *req, HttpResp *resp)
    {
        resp->MySQL("mysql://root:111111@localhost", "SHOW DATABASES");

    });

    svr.GET("/mysql01", [](const HttpReq *req, HttpResp *resp)
    {
        std::string url = "mysql://root:111111@localhost";
        resp->MySQL(url, "SHOW DATABASES", [resp](Json *json) 
        {
            Json js;
            js["rows"] = (*json)["result_set"][0]["rows"];
            resp->String(js.dump());
        });
    });

    svr.GET("/mysql02", [](const HttpReq *req, HttpResp *resp)
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
        getchar();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server\n");
        exit(1);
    }
    return 0;
}
```