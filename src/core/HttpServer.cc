#include "workflow/HttpMessage.h"

#include <utility>

#include "HttpServer.h"
#include "HttpServerTask.h"
#include "UriUtil.h"
#include "HttpFile.h"
#include "PathUtil.h"
#include "Router.h"
#include "ErrorCode.h"
#include "CodeUtil.h"

using namespace wfrest;

void HttpServer::process(HttpTask *task)
{
    auto *server_task = static_cast<HttpServerTask *>(task);
    server_task->server = this;
    auto *req = server_task->get_req();
    auto *resp = server_task->get_resp();
    const char *request_uri;
    std::string uri_str;

    req->fill_header_map();
    req->fill_content_type();

    const std::string &host = req->header("Host");

    if (host.empty() || host.find_first_of("/?#") != std::string::npos)
    {
        resp->set_status(HttpStatusBadRequest);
        return;
    }

    request_uri = req->get_request_uri();
	if (strncasecmp(request_uri, "http://", 7) == 0 ||
        strncasecmp(request_uri, "https://", 8) == 0)
    {
        uri_str = request_uri;
    }
    else if (*request_uri == '/')
    {
        const char *scheme = this->get_ssl_ctx() ? "https://" : "http://";
        uri_str = scheme + host + req->get_request_uri();
    }
    else
    {
        resp->set_status(HttpStatusBadRequest);
        return;
    }

    ParsedURI uri;
    if (URIParser::parse(uri_str, uri) < 0)
    {
        resp->set_status(HttpStatusBadRequest);
        return;
    }
    if (!uri.path)
        uri.path = strdup("/");

    std::string route("/");
    const char *pos = uri.path;
    while (*pos)
    {
        const char *slash = pos;
        while (*slash && *slash != '/')
            slash++;

        size_t n = slash - pos;
        if (n == 0 || (n == 1 && *pos == '.'))
            ;
        else if (n == 2 && *pos == '.' && pos[1] == '.')
        {
            if (route.size() > 1)
            {
                n = route.find_last_of('/', route.size() - 2);
                route.resize(n + 1);
            }
        }
        else
        {
            route.append(pos, slash);
            if (*slash)
                route.push_back('/');
        }

        if (!*slash)
            break;

        pos = slash + 1;
    }

    if (uri.query)
    {
        StringPiece query(uri.query);
        req->set_query_params(UriUtil::split_query(query));
    }

    req->set_parsed_uri(std::move(uri));
    std::string verb = req->get_method();
    int ret = blue_print_.router().call(str_to_verb(verb), CodeUtil::url_encode(route), server_task);
    if(ret != StatusOK && !default_route_.empty())
    {
        ret = blue_print_.router().call(str_to_verb(verb), CodeUtil::url_encode(default_route_), server_task);
    }
    if (ret != StatusOK) {
        resp->Error(ret, verb + " " + route);
    }
    if(track_func_)
    {
        server_task->add_callback(track_func_);
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

void HttpServer::register_blueprint(const BluePrint &bp, const std::string& url_prefix)
{
    blue_print_.add_blueprint(bp, url_prefix);
}

// /static : /www/file/
void HttpServer::Static(const char *relative_path, const char *root)
{
    BluePrint bp;
    int ret = serve_static(root, bp);
    if(ret != StatusOK)
    {
        fprintf(stderr, "[WFREST] Error : %s dose not exists\n", root);
        return;
    }
    blue_print_.add_blueprint(std::move(bp), relative_path);
}

void HttpServer::CachedStatic(const char *relative_path, const char *root)
{
    BluePrint bp;
    int ret = serve_static_cached(root, bp);
    if(ret != StatusOK)
    {
        fprintf(stderr, "[WFREST] Error : %s dose not exists\n", root);
        return;
    }
    blue_print_.add_blueprint(std::move(bp), relative_path);
}

int HttpServer::serve_static(const char* path, BluePrint &bp)
{
    std::string path_str(path);
    bool is_file = true;
    if (PathUtil::is_dir(path_str))
    {
        is_file = false;
    } else if(!PathUtil::is_file(path_str))
    {
        return StatusNotFound;
    }
    std::string route = "";
    if (!is_file)
    {
        route = "/*";
    }
    bp.GET(route, [path_str, is_file](const HttpReq *req, HttpResp *resp) {
        std::string match_path = req->match_path();
        if(is_file)
        {
            resp->File(path_str);
        } else
        {
            resp->File(path_str + "/" + match_path);
        }
    });
    return StatusOK;
}

int HttpServer::serve_static_cached(const char* path, BluePrint &bp)
{
    std::string path_str(path);
    bool is_file = true;
    if (PathUtil::is_dir(path_str))
    {
        is_file = false;
    } else if(!PathUtil::is_file(path_str))
    {
        return StatusNotFound;
    }
    std::string route = "";
    if (!is_file)
    {
        route = "/*";
    }
    bp.GET(route, [path_str, is_file](const HttpReq *req, HttpResp *resp) {
        std::string match_path = req->match_path();
        if(is_file)
        {
            resp->CachedFile(path_str);
        } else
        {
            resp->CachedFile(path_str + "/" + match_path);
        }
    });
    return StatusOK;
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
        fprintf(stderr, "[WFREST] %s | %s | %s : %d | %s | \"%s\" | -- \n",
                    fmt_time.c_str(),
                    resp->get_status_code(),
                    task->peer_addr().c_str(),
                    task->peer_port(),
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

