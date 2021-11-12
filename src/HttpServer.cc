#include <cstring>
#include <utility>
#include "workflow/HttpUtil.h"
#include "workflow/HttpMessage.h"
#include "HttpServer.h"
#include "HttpServerTask.h"
#include "UriUtil.h"
#include "Global.h"

using namespace wfrest;

void HttpServer::proc(WebTask *server_task)
{
    auto *req = server_task->get_req();
    auto *resp = server_task->get_resp();
    // conntect msg to server task
    req->set_task(server_task);
    resp->set_task(server_task);

    req->set_header_map(new protocol::HttpHeaderMap(req));

    std::string host = req->header("Host");

    if (host.empty())
    {
        //header Host not found
        resp->set_status(HttpStatusBadRequest);
        return;
    }

    std::string request_uri = "http://" + host + req->get_request_uri();    // or can't parse URI
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

    if (uri.query)
    {
        StringPiece query(uri.query);
        req->set_query_params(UriUtil::split_query(query));
    }

    req->parse_body();
    req->set_parsed_uri(std::move(uri));

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

void HttpServer::mount(std::string&& path)
{
    Global::get_http_file()->mount(std::move(path));
}





