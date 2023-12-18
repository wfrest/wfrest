#include "wfrest/HttpServerTask.h"
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
        // sse header required
        resp->add_header("Content-Type", "text/event-stream");
        resp->add_header("Cache-Control", "no-cache");
        resp->add_header("Connection", "keep-alive");
        resp->Push("test", [](std::string &body) {
            body.reserve(128);
            body.append(": ");
            body.append("comment");
            body.append("\n");
            body.append("id: ");
            body.append(std::to_string(StockPrice::id()));
            body.append("\n");
            body.append("event: ");
            body.append("message");
            body.append("\n");
            body.append("data: ");
            body.append("price|" + std::to_string(StockPrice::price()));
            body.append("\n\n");
        });
    });

    svr.GET("/sse_close", [](const HttpReq *req, HttpResp *resp)
    {
        // sse header required
        resp->add_header("Content-Type", "text/event-stream");
        resp->add_header("Cache-Control", "no-cache");
        resp->add_header("Connection", "keep-alive");
        resp->Push("test", [](std::string &body) {
            auto id = StockPrice::id();
            if (id > 10)
            {
                body.clear();
                // body.append("event: ");
                // body.append("close");
                // body.append("\n");
                // body.append("data: ");
                // body.append("\n\n");
            } else
            {
                body.append("id: ");
                body.append(std::to_string(id));
                body.append("\n");
                body.append("data: ");
                body.append("price : " + std::to_string(StockPrice::price()));
                body.append("\n\n");
            }
        });
    });

    svr.GET("/sse_handle_err", [](const HttpReq *req, HttpResp *resp)
    {
        // sse header required
        resp->add_header("Content-Type", "text/event-stream");
        resp->add_header("Cache-Control", "no-cache");
        resp->add_header("Connection", "keep-alive");
        resp->Push("test", [](std::string &body) {
            auto id = StockPrice::id();
            if (id > 10)
            {
                body.clear();
            } else
            {
                body.append("id: ");
                body.append(std::to_string(id));
                body.append("\n");
                body.append("data: ");
                body.append("price : " + std::to_string(StockPrice::price()));
                body.append("\n\n");
            }
        }, [] {
            fprintf(stderr, "sse_handle_err\n");
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
