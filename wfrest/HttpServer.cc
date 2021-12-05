#include "workflow/HttpUtil.h"
#include "workflow/HttpMessage.h"

#include <utility>

#include "wfrest/HttpServer.h"
#include "wfrest/HttpServerTask.h"
#include "wfrest/UriUtil.h"
#include "wfrest/Logger.h"
#include "wfrest/HttpFile.h"

using namespace wfrest;

void HttpServer::process(HttpTask *task)
{
    auto *server_task = static_cast<HttpServerTask *>(task);

    auto *req = server_task->get_req();
    auto *resp = server_task->get_resp();

    auto *header_map_ptr = new protocol::HttpHeaderMap(req);
    req->set_header_map(header_map_ptr);
    server_task->add_callback([header_map_ptr](const HttpTask *)
    {
        delete header_map_ptr;
    });

    std::string host = req->header("Host");
    
    if (host.empty())
    {
        //header Host not found
        resp->set_status(HttpStatusBadRequest);
        return;
    }
    
    std::string request_uri = "http://" + host + req->get_request_uri();  // or can't parse URI
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
    req->fill_content_type();

    router_.call(req->get_method(), route, server_task);
}


void HttpServer::GET(const char *route, const Handler &handler)
{
    router_.handle(route, -1, handler, nullptr, Verb::GET);
}

void HttpServer::GET(const char *route, const SeriesHandler &series_handler)
{
    router_.handle(route, -1, nullptr, series_handler, Verb::GET);
}

void HttpServer::GET(const char *route, int compute_queue_id, const Handler &handler)
{
    router_.handle(route, compute_queue_id, handler, nullptr, Verb::GET);
}

void HttpServer::GET(const char *route, int compute_queue_id, const SeriesHandler &series_handler)
{
    router_.handle(route, compute_queue_id, nullptr, series_handler, Verb::GET);
}

void HttpServer::POST(const char *route, const Handler &handler)
{
    router_.handle(route, -1, handler, nullptr, Verb::POST);
}

void HttpServer::POST(const char *route, const SeriesHandler &series_handler)
{
    router_.handle(route, -1, nullptr, series_handler, Verb::POST);
}

void HttpServer::POST(const char *route, int compute_queue_id, const Handler &handler)
{
    router_.handle(route, compute_queue_id, handler, nullptr, Verb::POST);
}

void HttpServer::POST(const char *route, int compute_queue_id, const SeriesHandler &series_handler)
{
    router_.handle(route, compute_queue_id, nullptr, series_handler, Verb::POST);
}

CommSession *HttpServer::new_session(long long seq, CommConnection *conn)
{
    HttpTask *task = new HttpServerTask(this, this->WFServer<HttpReq, HttpResp>::process);
    task->set_keep_alive(this->params.keep_alive_timeout);
    task->set_receive_timeout(this->params.receive_timeout);
    task->get_req()->set_size_limit(this->params.request_size_limit);

    return task;
}

void HttpServer::mount(std::string &&path)
{
    HttpFile::mount(std::move(path));
}

