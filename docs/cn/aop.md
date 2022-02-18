## 面向切面编程AOP

In computing, aspect-oriented programming (AOP) is a programming paradigm that aims to increase modularity by allowing the separation of cross-cutting concerns.

For more information, you can see [What is AOP](https://en.wikipedia.org/wiki/Aspect-oriented_programming),

```cpp
#include "wfrest/HttpServer.h"
#include "wfrest/Aspect.h"
using namespace wfrest;

// Logging aspect
struct LogAop : public Aspect
{
    bool before(const HttpReq *req, HttpResp *resp) override 
    {
        fprintf(stderr, "before \n");
        return true;
    }

    // 'after()' should be called after reply
    bool after(const HttpReq *req, HttpResp *resp) override
    {
        fprintf(stderr, "After log\n");
        return true;
    }
};

int main()
{
    HttpServer svr;

    svr.GET("/aop", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("aop");
    }, LogAop());

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

从切片传输数据到http handler

```cpp
#include "wfrest/HttpServer.h"
#include "wfrest/Aspect.h"
using namespace wfrest;

struct TransferAop : public Aspect
{
    bool before(const HttpReq *req, HttpResp *resp) override 
    {
        auto *content = new std::string("transfer data");
        resp->user_data = content;
        return true;
    }

    // If resp's 'user_data' needs to be deleted, delete it in 'after()'.
    bool after(const HttpReq *req, HttpResp *resp) override
    { 
        fprintf(stderr, "state : %d\terror : %d\n", 
                resp->get_state(), resp->get_error());
        delete static_cast<std::string *>(resp->user_data);
        return true;
    }
};

int main()
{
    HttpServer svr;

    svr.GET("/aop", [](const HttpReq *req, HttpResp *resp)
    {
        auto *content = static_cast<std::string *>(resp->user_data);
        resp->String(std::move(*content));
    }, TransferAop());

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

## Global Aspect

注册全局的切片:

```cpp
svr.Use(FirstAop());
svr.Use(SecondAop(), ThirdAop());
```