#include "workflow/HttpMessage.h"

#include <utility>

#include "wfrest/HttpServer.h"
#include "wfrest/HttpServerTask.h"
#include "wfrest/UriUtil.h"
#include "wfrest/HttpFile.h"
#include "wfrest/PathUtil.h"
#include "wfrest/Macro.h"
#include "wfrest/Router.h"
#include "wfrest/ErrorCode.h"

using namespace wfrest;

void HttpServer::process(HttpTask *task)
{
    auto *server_task = static_cast<HttpServerTask *>(task);

    auto *req = server_task->get_req();
    auto *resp = server_task->get_resp();
    
    req->fill_header_map();
    req->fill_content_type();

    const std::string &host = req->header("Host");
    
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

    req->set_parsed_uri(std::move(uri));
    std::string verb = req->get_method();
    int ret = blue_print_.router().call(verb, route, server_task);
    if(ret != StatusOK)
    {
        resp->Error(ret, verb + " " + route);
    }
    if(track_func_)
    {
        track_func_(server_task);
    }
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
    {
        fprintf(stderr, "Serve Dir failed. Directory %s dose not exists\n", dir_path);
        return;
    }
    
    // Check if it is a directory.
    // todo : copy here
    if (!PathUtil::is_dir(realpath_out))
    {
        fprintf(stderr, "Serve Dir failed. %s is not a directory\n", dir_path);
        return;
    }
        
    std::string real_root(realpath_out);
    if(real_root.back() != '/')
    {
        real_root.push_back('/');
    }

    bp.GET("/*", [real_root](const HttpReq *req, HttpResp *resp) {
        std::string path = real_root + req->match_path();
        // todo : file_exist
        // fprintf(stderr, "Get File path : %s\n", path.c_str()); 
        resp->File(path);
    });
}

HttpServer &HttpServer::track()
{
    track_func_ = [](HttpTask *server_task) {
        HttpResp *resp = server_task->get_resp();
        HttpReq *req = server_task->get_req();
        HttpServerTask *task = static_cast<HttpServerTask *>(server_task);
        Timestamp current_time = Timestamp::now();
        std::string fmt_time = current_time.to_format_str();

        // time | http status code | peer ip address | verb | route path
        fprintf(stderr, "[WFREST] %s | %d | %s | %s | %s |\n", 
                    fmt_time.c_str(),
                    resp->status_code(),
                    task->get_peer_addr_str().c_str(), 
                    req->get_method(),
                    req->current_path().c_str());
    };
    return *this;
}

HttpServer &HttpServer::track(const TrackFunc &track_func)
{
    track_func_ = track_func;
    return *this;
}

HttpServer &HttpServer::track(TrackFunc &&track_func)
{
    track_func_ = std::move(track_func);
    return *this;
}