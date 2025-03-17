## Https 服务器

设置Https服务器非常简单。

http和https之间的唯一区别是https需要您提供SSL密钥的路径和SSL证书的路径。

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main(int argc, char **argv)
{
    // 在cert文件中 
    // 执行sudo ./gen.sh 生成crt / key文件
    if (argc != 3)
    {
        fprintf(stderr, "%s [cert file] [key file]\n",
                argv[0]);
        exit(1);
    }

    HttpServer svr;

    svr.GET("/https", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("测试Https\n");
    });

    if (svr.start(8888, argv[1], argv[2]) == 0)
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