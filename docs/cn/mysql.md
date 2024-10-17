## MySQL

MySQL接口目前支持多种模式

基础模式是暴露json接口，可以选择自己剪枝来发送或处理。

还有cursor接口，更加高效灵活操作MySQL返回的结果。

```cpp
#include "workflow/MySQLResult.h"
#include "wfrest/HttpServer.h"

using namespace wfrest;
using namespace protocol;

int main(int argc, char **argv)
{
    HttpServer svr;
 
    // 1. 默认返回json
    svr.GET("/mysql00", [](const HttpReq *req, HttpResp *resp)
    {
        resp->MySQL("mysql://root:111111@localhost", "SHOW DATABASES");
    });

    // 2. 自行剪枝并发送
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

    // 3. 通过cursor遍历
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

    // 4. 多条语句组合
    svr.GET("/multi", [](const HttpReq *req, HttpResp *resp)
    {
        resp->MySQL("mysql://root:111111@localhost/wfrest_test", "SHOW DATABASES; SELECT * FROM wfrest");
    });

    // 5. 一个简单的客户端接口
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

## 参数1 : MySQL URL的格式

mysql://username:password@host:port/dbname?character_set=charset&character_set_results=charset

- 如果以SSL连接访问MySQL，则scheme设为**mysqls://**。MySQL server 5.7及以上支持；

- username和password按需填写；

- port默认为3306；

- dbname为要用的数据库名，一般如果SQL语句只操作一个db的话建议填写；

- character_set为client的字符集，等价于使用官方客户端启动时的参数``--default-character-set``的配置，默认utf8，具体可以参考MySQL官方文档[character-set.html](https://dev.mysql.com/doc/internals/en/character-set.html)。

- character_set_results为client、connection和results的字符集，如果想要在SQL语句里使用``SET NAME``来指定这些字符集的话，请把它配置到url的这个位置。

MySQL URL示例：

mysql://root:password@127.0.0.1

mysql://@test.mysql.com:3306/db1?character_set=utf8&character_set_results=utf8

mysqls://localhost/db1?character\_set=big5

## 参数2 : sql语句

目前支持的命令为COM_QUERY，已经能涵盖用户基本的增删改查、建库删库、建表删表、预处理、使用存储过程和使用事务的需求。

因为我们的交互命令中不支持选库（USE命令），所以，如果SQL语句中有涉及到跨库的操作，则可以通过db_name.table_name的方式指定具体哪个库的哪张表。

其他所有命令都可以拼接到一起传给第二个参数包括 INSERT/UPDATE/SELECT/DELETE/PREPARE/CALL。

拼接的命令会被按序执行直到命令发生错误，前面的命令会执行成功。

```cpp
resp->MySQL("mysql://root:111111@localhost/wfrest_test", "SHOW DATABASES; SELECT * FROM wfrest");
```

## 参数3 ：异步回调函数

1. 无第三个参数，默认是返回整个json串

resp->MySQL("mysql://root:111111@localhost", "SHOW DATABASES");

2. json callback

```cpp
using MySQLJsonFunc = std::function<void(Json *json)>;
```

json指针是完整的json串，可以自行剪枝处理。

```cpp
resp->MySQL(url, "SHOW DATABASES", [resp](Json *json) 
{
    Json js;
    js["rows"] = (*json)["result_set"][0]["rows"];
    resp->String(js.dump());
});
```

3. cursor callback

```cpp
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
```

我们可以通过MySQLResultCursor遍历结果集。






