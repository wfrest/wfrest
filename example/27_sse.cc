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

    svr.GET("/notify", [](const HttpReq *req, HttpResp *resp)
    {
        sse_signal("test"); 
    });

    svr.GET("/interval", [](const HttpReq *req, HttpResp *resp)
    {
        int cnt = 0;
        while (true)
        {
            sse_signal("test"); 
            sleep(1);
            if (cnt++ == 10)
            {
                break;
            }
        }
    });

    svr.GET("/sse", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Push("test", [](std::vector<SseContext>& ctx_list) {
            SseContext sse_ctx;
            sse_ctx.id = std::to_string(StockPrice::id());
            sse_ctx.event = "message";
            sse_ctx.data = "price : " + std::to_string(StockPrice::price());
            ctx_list.emplace_back(std::move(sse_ctx));
        });
    });

    svr.GET("/sse_json", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Push("json_test", [](Json& js) {
            js["id"] = std::to_string(StockPrice::id()); 
            js["data"] = "data";
        });
    });

    svr.GET("/sse_json_arr", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Push("multi_json_test", [](Json& js) {
            Json data1;
            data1["id"] = std::to_string(StockPrice::id()); 
            data1["data"] = "data1";
            js.push_back(data1);
            Json data2;
            data2["data"] = "data2";
            js.push_back(data2);
            Json data3;
            data3["data"] = "data3";
            js.push_back(data3);
        });
    });

    svr.GET("/sse_close", [](const HttpReq *req, HttpResp *resp)
    {
        int cnt = 0;
        resp->Push("test", [&cnt](Json& js) {
            if (++cnt == 10)
            {
                js["event"] = "close";
                js["data"] = "";
            } else 
            {
                js["id"] = std::to_string(StockPrice::id()); 
                js["data"] = "data";
            }
        });
    });

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
