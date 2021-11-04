#include <cstring>
#include <utility>
#include "workflow/HttpUtil.h"
#include "workflow/HttpMessage.h"
#include "HttpServer.h"
#include "HttpServerTask.h"

using namespace wfrest;


void HttpServer::proc(WebTask *server_task)
{
    auto *req = server_task->get_req();
    auto *resp = server_task->get_resp();

    std::string host;
    protocol::HttpHeaderCursor cursor(req);
    cursor.find("Host", host);
    fprintf(stderr, "Host : %s\n", host.c_str());
    if (host.empty())
    {
        //header Host not found
        resp->set_status(HttpStatusBadRequest);
        return;
    }

    std::string request_uri = "http://";    // or can't parse URI

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

    fprintf(stderr, "%s\n", uri.path);
    router_.call(req->get_method(), route, req, resp);
}


void HttpServer::Get(const char *route, const Handler &handler)
{
    router_.handle(route, handler, GET);
}

void HttpServer::Post(const char *route, const Handler &handler)
{
    router_.handle(route, handler, POST);
}

CommSession *HttpServer::new_session(long long seq, CommConnection *conn)
{
    WebTask *task = new HttpServerTask(this, this->process);
    task->set_keep_alive(this->params.keep_alive_timeout);
    task->set_receive_timeout(this->params.receive_timeout);
    task->get_req()->set_size_limit(this->params.request_size_limit);

    return task;
}





