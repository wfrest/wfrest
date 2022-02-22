## Redis

```cpp
#include "wfrest/HttpServer.h"
#include "wfrest/json.hpp"

using Json = nlohmann::json;
using namespace wfrest;

int main(int argc, char **argv)
{
    HttpServer svr;
 
    svr.GET("/redis0", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Redis("redis://127.0.0.1/", "SET", {"test_val", "test_key"});
    });

    svr.GET("/redis1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Redis("redis://127.0.0.1/", "GET", {"test_val"});
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

返回默认格式为json

## Redis URL的格式

redis://:password@host:port/dbnum?query#fragment  
如果是SSL，则为：  
rediss://:password@host:port/dbnum?query#fragment  
password是可选项。port的缺省值是6379，dbnum缺省值0，范围0-15。  
query和fragment部分工厂里不作解释，用户可自行定义。比如，用户有upstream选取需求，可以自定义query和fragment。相关内容参考upstream文档。  
redis URL示例：  
redis://127.0.0.1/  
redis://:12345678@redis.some-host.com/1


