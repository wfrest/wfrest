#ifndef WFREST_HTTPSERVER_H_
#define WFREST_HTTPSERVER_H_

#include "workflow/WFHttpServer.h"
#include "workflow/HttpUtil.h"

#include <unordered_map>
#include <string>

#include "wfrest/HttpMsg.h"
#include "wfrest/BluePrint.h"

namespace wfrest
{

class HttpServer : public WFServer<HttpReq, HttpResp>, public Noncopyable
{
public:
    void GET(const char *route, const Handler &handler);

    void GET(const char *route, int compute_queue_id, const Handler &handler);

    void POST(const char *route, const Handler &handler);

    void POST(const char *route, int compute_queue_id, const Handler &handler);

public:
    void GET(const char *route, const SeriesHandler &handler);

    void GET(const char *route, int compute_queue_id, const SeriesHandler &handler);

    void POST(const char *route, const SeriesHandler &handler);

    void POST(const char *route, int compute_queue_id, const SeriesHandler &handler);

public:
    void Static(const char *relative_path, const char *root);

    void list_routes();

    void register_blueprint(const BluePrint &bp, const std::string &url_prefix);

public:
    HttpServer() :
            WFServer(std::bind(&HttpServer::process, this, std::placeholders::_1))
    {}

    HttpServer &set_max_connection(size_t max_connections)
    {
        this->params.max_connections = max_connections;
        return *this;
    }

    HttpServer &set_peer_response_timeout(int peer_response_timeout)
    {
        this->params.peer_response_timeout = peer_response_timeout;
        return *this;
    }

    HttpServer &set_receive_timeout(int receive_timeout)
    {
        this->params.receive_timeout = receive_timeout;
        return *this;
    }

    HttpServer &set_keep_alive_timeout(int keep_alive_timeout)
    {
        this->params.keep_alive_timeout = keep_alive_timeout;
        return *this;
    }

    HttpServer &set_request_size_limit(size_t request_size_limit)
    {
        this->params.request_size_limit = request_size_limit;
        return *this;
    }

    HttpServer &set_ssl_accept_timeout(int ssl_accept_timeout)
    {
        this->params.ssl_accept_timeout = ssl_accept_timeout;
        return *this;
    }

protected:
    CommSession *new_session(long long seq, CommConnection *conn) override;

private:
    void process(HttpTask *task);

    void serve_dir(const char *dir_path, OUT BluePrint &bp);

private:
    BluePrint blue_print_;
};

}  // namespace wfrest

#endif // WFREST_HTTPSERVER_H_