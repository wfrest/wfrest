## 计算处理器

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

void Fibonacci(int n, HttpResp *resp)
{
    unsigned long long x = 0, y = 1;
    if (n <= 0 || n > 94)
    {
        fprintf(stderr, "invalid parameter");
        return;
    }
    for (int i = 2; i < n; i++)
    {
        y = x + y;
        x = y - x;
    }
    if (n == 1)
        y = 0;
    resp->String("fib(" + std::to_string(n) + ") is : " + std::to_string(y) + "\n");
}

int main()
{
    HttpServer svr;
    // 第二个参数表示此计算队列ID为1
    // 然后这个处理器成为一个计算任务
    // curl -v http://ip:port/compute_task?num=20
    svr.GET("/compute_task", 1, [](const HttpReq *req, HttpResp *resp)
    {
        int num = std::stoi(req->query("num"));
        Fibonacci(num, resp);
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

## 计算任务接口

如果您想启动一个计算任务，只需使用`resp->Compute()`

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

void Factorial(int n, HttpResp *resp)
{
    unsigned long long factorial = 1;
    for(int i = 1; i <= n; i++)
    {
        factorial *= i;
    }
    resp->String("fac(" + std::to_string(n) + ") = " + std::to_string(factorial));
}

int main()
{
    HttpServer svr;

    svr.GET("/compute", [](const HttpReq *req, HttpResp *resp)
    {
        // 第一个参数是队列ID
        resp->Compute(1, Factorial, 10, resp);
    });

    if (svr.track().start(8888) == 0)
    {
        svr.list_routes();
        wait_group.wait();
        getchar();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
``` 