#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"
#include "wfrest/Aspect.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

// Logging aspect
struct LogAop : public Aspect
{
	bool before(const HttpReq *req, HttpResp *resp) override 
    {
        fprintf(stderr, "before log\n");
		return true;
	}

	bool after(const HttpReq *req, HttpResp *resp) override
    {
		fprintf(stderr, "After log\n");
		return true;
	}
};

struct OtherAop : public Aspect
{
	bool before(const HttpReq *req, HttpResp *resp) override 
    {
        fprintf(stderr, "before other\n");
		return true;
	}

    // 'after()' should be called after reply
	bool after(const HttpReq *req, HttpResp *resp) override
    {
		fprintf(stderr, "After other\n");
        fprintf(stderr, "state : %d\terror : %d\n", 
                    resp->get_state(), resp->get_error());
		return true;
	}
};

// transfer data from aspect to http handler
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
        delete static_cast<std::string *>(resp->user_data);
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
    
    svr.GET("/no_aop", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("no aop");
    });

    svr.GET("/aop", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("aop");
    }, LogAop());

    svr.GET("/more_aop", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("more aop");
    }, LogAop(), OtherAop());

    svr.GET("/data", [](const HttpReq *req, HttpResp *resp)
    {
        auto *content = static_cast<std::string *>(resp->user_data);
        resp->String(std::move(*content));
    }, TransferAop());

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