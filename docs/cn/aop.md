## 面向切面编程

在计算机领域，面向切面编程（AOP）是一种旨在通过允许分离横切关注点来增加模块化的编程范式。

有关更多信息，您可以查看 [什么是AOP](https://en.wikipedia.org/wiki/Aspect-oriented_programming)，

```cpp
#include "wfrest/HttpServer.h"
#include "wfrest/Aspect.h"
using namespace wfrest;

// 日志切面
struct LogAop : public Aspect
{
    bool before(const HttpReq *req, HttpResp *resp) override 
    {
        fprintf(stderr, "before \n");
        return true;
    }

    // 'after()' 应该在回复后被调用
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

从切面传输数据到HTTP处理器：

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

    // 如果需要删除resp的'user_data'，在'after()'中删除它。
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

## 全局切面

注册全局切面的方式如下：

```cpp
svr.Use(FirstAop());
svr.Use(SecondAop(), ThirdAop());
``` 