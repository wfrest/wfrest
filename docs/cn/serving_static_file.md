## 提供静态文件

```cpp
#include "wfrest/HttpServer.h"

using namespace wfrest;

int main()
{
    HttpServer svr;
    svr.Static("/static", "./www/static");

    svr.Static("/public", "./www");

    svr.Static("/", "./www/index.html");
    
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