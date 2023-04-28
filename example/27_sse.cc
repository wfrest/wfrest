#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

class StockPrice
{
public:
    static int price()
    {
        srand(time(nullptr));
        return rand() % 100;
    }
    static int id()
    {
        return id_++;
    }
private:
    static int id_;
};

int StockPrice::id_ = 0;

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;


    svr.GET("/sse", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Push(1000000, [](std::vector<SseContext>* ctx_list) {
            SseContext sse_ctx;
            sse_ctx.id = std::to_string(StockPrice::id());
            sse_ctx.event = "message";
            sse_ctx.data = "price : " + std::to_string(StockPrice::price());
            ctx_list->emplace_back(std::move(sse_ctx));
        });
    });

    /*
    svr.GET("/sse", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Push(1000000, [](Json* js) {
            js["id"] = id++;
            js["data"] = "data";
        });
    });
    */
    if (svr.track().start(8888) == 0)
    {
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}