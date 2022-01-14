#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"
#include "wfrest/Aspect.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

// Logging aspect
struct FirstAop : public Aspect
{
	bool before(const HttpReq *req, HttpResp *resp) override 
    {
        fprintf(stderr, "before 111\n");
		return true;
	}

	bool after(const HttpReq *req, HttpResp *resp) override
    {
		fprintf(stderr, "After 111\n");
		return true;
	}
};

struct SecondAop : public Aspect
{
	bool before(const HttpReq *req, HttpResp *resp) override 
    {
        fprintf(stderr, "before 222\n");
		return true;
	}

	bool after(const HttpReq *req, HttpResp *resp) override
    {
		fprintf(stderr, "After 222\n");
		return true;
	}
};

struct ThirdAop : public Aspect
{
	bool before(const HttpReq *req, HttpResp *resp) override 
    {
        fprintf(stderr, "before 333\n");
		return true;
	}

	bool after(const HttpReq *req, HttpResp *resp) override
    {
		fprintf(stderr, "After 333\n");
		return true;
	}
};

struct FourthAop : public Aspect
{
	bool before(const HttpReq *req, HttpResp *resp) override 
    {
        fprintf(stderr, "before 444\n");
		return true;
	}

	bool after(const HttpReq *req, HttpResp *resp) override
    {
		fprintf(stderr, "After 444\n");
		return true;
	}
};


void sig_handler(int signo)
{
    wait_group.done();
}

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;
    
    svr.Use(FirstAop());
    svr.Use(SecondAop(), ThirdAop());

    svr.GET("/aop", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("aop");
    });

    svr.GET("/more_aop", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("aop + global aop");
    }, FourthAop());

    if (svr.start(8888) == 0)
    {
        svr.list_routes();
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}