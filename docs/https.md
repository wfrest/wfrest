## Https Server

It is easy to set up an Https Server. 

The only difference between http and https is that https require you to provide the path of the SSL key and path of the SSL certificate.

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main(int argc, char **argv)
{
    // in cert file 
    // sudo ./gen.sh to generate crt / key files
    if (argc != 3)
    {
        fprintf(stderr, "%s [cert file] [key file]\n",
                argv[0]);
        exit(1);
    }

    HttpServer svr;

    svr.GET("/https", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("Test Https\n");
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