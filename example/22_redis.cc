#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main(int argc, char **argv)
{
    signal(SIGINT, sig_handler);

    HttpServer svr;

    svr.GET("/redis0", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Redis("redis://127.0.0.1/", "SET", {"test_val", "test_key"});
    });

    svr.GET("/redis1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Redis("redis://127.0.0.1/", "GET", {"test_val"});
    });

    svr.GET("/redis1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->Redis("redis://127.0.0.1/", "GET", {"test_val"}, [resp](WFRedisTask *redis_task) {
            protocol::RedisRequest *redis_req = redis_task->get_req();
            protocol::RedisResponse *redis_resp = redis_task->get_resp();
            int state = redis_task->get_state();
            int error = redis_task->get_error();
            protocol::RedisValue val;
            wfrest::Json js;
            switch (state)
            {
            case WFT_STATE_SYS_ERROR:
                js["errmsg"] = "system error: " + std::string(strerror(error));
                break;
            case WFT_STATE_DNS_ERROR:
                js["errmsg"] = "DNS error: " + std::string(gai_strerror(error));
                break;
            case WFT_STATE_SSL_ERROR:
                js["errmsg"] = "SSL error: " + std::to_string(error);
                break;
            case WFT_STATE_TASK_ERROR:
                js["errmsg"] = "Task error: " + std::to_string(error);
                break;
            case WFT_STATE_SUCCESS:
                redis_resp->get_result(val);
                if (val.is_error())
                {
                    js["errmsg"] = "Error reply. Need a password?\n";
                    state = WFT_STATE_TASK_ERROR;
                }
                break;
            }
            std::string cmd;
            std::vector<std::string> params;
            redis_req->get_command(cmd);
            redis_req->get_params(params);
            if(state == WFT_STATE_SUCCESS && cmd == "GET")
            {
                js["cmd"] = "GET";
                if (val.is_string())
                {
                    js[params[0]] = val.string_value();
                    js["status"] = "success";
                }
                else
                {
                    js["errmsg"] = "value is not a string value";
                }
            }
            resp->Json(js);
        });
    });

    if (svr.start(8888) == 0)
    {
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server\n");
        exit(1);
    }
    return 0;
}

