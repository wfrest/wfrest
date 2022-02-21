## 面向切面编程AOP

AOP为Aspect Oriented Programming的缩写，意为：面向切面编程，通过预编译方式和运行期间动态代理实现程序功能的统一维护的一种技术。对业务逻辑的各个部分进行隔离，从而使得业务逻辑各部分之间的耦合度降低，提高程序的可重用性，同时提高了开发的效率。

限于C++语言的特性，wfrest没有提供像Spring那样灵活的AOP方案，而是一种简单的AOP，所有插入点都是内建于框架中的，提供了两个插入点，一个是`handler`之前的`before()`, 一个在server task执行结束后的`after()`中。

## 示例1

日志切面

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

## 示例2

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

## 全局切面

注册全局的切片:

```cpp
svr.Use(FirstAop());
svr.Use(SecondAop(), ThirdAop());
```