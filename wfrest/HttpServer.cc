#include "workflow/HttpMessage.h"

#include <utility>

#include "wfrest/HttpServer.h"
#include "wfrest/HttpServerTask.h"
#include "wfrest/UriUtil.h"
#include "wfrest/Logger.h"
#include "wfrest/HttpFile.h"
#include "wfrest/PathUtil.h"
#include "wfrest/Macro.h"
#include "wfrest/Router.h"

using namespace wfrest;

void HttpServer::process(HttpTask *task)
{
    auto *server_task = static_cast<HttpServerTask *>(task);

    auto *req = server_task->get_req();
    auto *resp = server_task->get_resp();

    req->fill_content_type();
    req->fill_header_map();

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
    blue_print_.router().call(req->get_method(), route, server_task);
}


void HttpServer::GET(const char *route, const Handler &handler)
{
    blue_print_.GET(route, handler);
}

void HttpServer::GET(const char *route, int compute_queue_id, const Handler &handler)
{
    blue_print_.GET(route, compute_queue_id, handler);
}

void HttpServer::POST(const char *route, const Handler &handler)
{
    blue_print_.POST(route, handler);
}

void HttpServer::POST(const char *route, int compute_queue_id, const Handler &handler)
{
    blue_print_.POST(route, compute_queue_id, handler);
}

void HttpServer::GET(const char *route, const SeriesHandler &series_handler)
{
    blue_print_.GET(route, series_handler);
}

void HttpServer::GET(const char *route, int compute_queue_id, const SeriesHandler &series_handler)
{
    blue_print_.GET(route, compute_queue_id, series_handler);
}

void HttpServer::POST(const char *route, const SeriesHandler &series_handler)
{
    blue_print_.POST(route, series_handler);
}

void HttpServer::POST(const char *route, int compute_queue_id, const SeriesHandler &series_handler)
{
    blue_print_.POST(route, compute_queue_id, series_handler);
}

CommSession *HttpServer::new_session(long long seq, CommConnection *conn)
{
    HttpTask *task = new HttpServerTask(this, this->WFServer<HttpReq, HttpResp>::process);
    task->set_keep_alive(this->params.keep_alive_timeout);
    task->set_receive_timeout(this->params.receive_timeout);
    task->get_req()->set_size_limit(this->params.request_size_limit);

    return task;
}

void HttpServer::list_routes()
{
    blue_print_.router().print_routes();
}

void HttpServer::register_blueprint(const BluePrint& bp, const std::string& url_prefix)
{
    blue_print_.add_blueprint(bp, url_prefix);
}

// /static : /www/file/
void HttpServer::Static(const char *relative_path, const char *root)
{
    BluePrint bp;
    serve_dir(root, bp);
    blue_print_.add_blueprint(bp, relative_path);
}

void HttpServer::serve_dir(const char* dir_path, OUT BluePrint &bp)
{
    // extract root realpath. 
    char realpath_out[PATH_MAX]{0};
    if (nullptr == realpath(dir_path, OUT realpath_out))
        LOG_SYSERR << "Directory " << dir_path << " does not exists.";
    
    // Check if it is a directory.
    if (!PathUtil::is_dir(realpath_out))
        LOG_SYSERR << dir_path << " is not a directory.";
        
    std::string real_root(realpath_out);
    if(real_root.back() != '/')
    {
        real_root.push_back('/');
    }

    bp.GET("/*", [real_root](const HttpReq *req, HttpResp *resp) {
        std::string path = real_root + req->match_path();
        LOG_DEBUG << "File path : " << path;
        resp->File(path);
    });
}
