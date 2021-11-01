#include <cstring>
#include <utility>
#include "workflow/HttpUtil.h"
#include "workflow/HttpMessage.h"
#include "WFWebServer.h"
#include "WFWebServerTask.h"

using namespace wfrest;


void WFWebServer::proc(WFWebTask *server_task)
{
    auto *req = server_task->get_req();
    auto *resp = server_task->get_resp();

    std::string host;
    protocol::HttpHeaderCursor cursor(req);
    cursor.find("Host", host);

    if (host.empty())
    {
        //header Host not found
        resp->set_status(HttpStatusBadRequest);
        return;
    }

    std::string request_uri;

    if (is_ssl_)
        request_uri = "https://";
    else
        request_uri = "http://";

    request_uri += host;
    request_uri += req->get_request_uri();

    ParsedURI uri;

    if (URIParser::parse(request_uri, uri) < 0)
    {
        resp->set_status(HttpStatusBadRequest);
        return;
    }

    std::string route;

    if (uri.path && uri.path[0])
        route = uri.path;
    else
        route = "/";

    router_.call(req->get_method(), route, req, resp);
}


class __WFHttpTask : public WFNetworkTask<HttpReq, HttpResp>
{
public:
    static size_t get_req_offset()
    {
        __WFHttpTask task(nullptr);

        return task.req_offset();
    }

    static size_t get_resp_offset()
    {
        __WFHttpTask task(nullptr);

        return task.resp_offset();
    }
    // Just for get rid of abtract
    WFConnection *get_connection() const override { return nullptr; }
    CommMessageOut *message_out() override { return &this->req;};
    CommMessageIn *message_in() override { return &this->resp; };
private:
    explicit __WFHttpTask(std::function<void(WFWebTask *)> proc) :
            WFNetworkTask(nullptr, nullptr, std::move(proc))
    {}

    size_t req_offset() const
    {
        return (const char *) (&this->req) - (const char *) this;
    }

    size_t resp_offset() const
    {
        return (const char *) (&this->resp) - (const char *) this;
    }
};

WFWebTask *WFWebServer::task_of(const HttpReq *req)
{
    size_t http_req_offset = __WFHttpTask::get_req_offset();
    return (WFWebTask *) ((char *) (req) - http_req_offset);
}

WFWebTask *WFWebServer::task_of(const HttpResp *resp)
{
    size_t http_resp_offset = __WFHttpTask::get_resp_offset();
    return (WFWebTask *) ((char *) (resp) - http_resp_offset);
}

CommSession *WFWebServer::new_session(long long seq, CommConnection *conn)
{
    WFWebTask *task;
    task = WFWebTaskFactory::create_web_task(this, this->process);
    task->set_keep_alive(this->params.keep_alive_timeout);
    task->set_receive_timeout(this->params.receive_timeout);
    task->get_req()->set_size_limit(this->params.request_size_limit);

    return task;
}

void WFWebServer::Get(std::string &&route, const WFWebServer::Handler &handler)
{
    router_.handle(std::move(route), handler, GET);
}

void WFWebServer::Post(std::string &&route, const WFWebServer::Handler &handler)
{
    router_.handle(std::move(route), handler, POST);
}






