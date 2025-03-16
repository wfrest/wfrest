## 路径中的参数

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // 此处理程序将匹配 /user/chanchan 但不会匹配 /user/ 或 /user
    // curl -v "ip:port/user/chanchan/"
    svr.GET("/user/{name}", [](const HttpReq *req, HttpResp *resp)
    {
        // 引用：不复制
        const std::string& name = req->param("name");
        // resp->set_status(HttpStatusOK); // 自动设置
        resp->String("你好 " + name + "\n");
    });

    // wildcast/chanchan/action... (前缀)
    svr.GET("/wildcast/{name}/action*", [](const HttpReq *req, HttpResp *resp)
    {
        const std::string& name = req->param("name");
        const std::string& match_path = req->match_path();

        resp->String("[name : " + name + "] [match path : " + match_path + "]\n");
    });

    // 请求将保存路由定义
    svr.GET("/user/{name}/match*", [](const HttpReq *req, HttpResp *resp)
    {
        // 如果 /user/chanchan/match1234
        // full_path : /user/{name}/match*
        // current_path : /user/chanchan/match1234
        // match_path : match1234
        const std::string &full_path = req->full_path();
        const std::string &current_path = req->current_path();
        std::string res;
        if (full_path == "/user/{name}/match*")
        {
            res = full_path + " 匹配 : " + current_path;
        } else
        {
            res = full_path + " 不匹配";
        }
        resp->String(res);
    });

    // 此处理程序将为 /user/groups 添加一个新路由。
    // 无论定义的顺序如何，精确路由都会在参数路由之前解析。
    // 以 /user/groups 开头的路由永远不会被解释为 /user/{name}/... 路由
    svr.GET("/user/groups", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String(req->full_path());
    });

    if (svr.start(8888) == 0)
    {
        getchar();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
``` 