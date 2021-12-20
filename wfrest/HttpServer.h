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
    void GET(const char *route, const Handler &handler)
    {
        blue_print_.GET(route, handler);
    }

    void GET(const char *route, int compute_queue_id, const Handler &handler)
    {
        blue_print_.GET(route, compute_queue_id, handler);
    }

    void POST(const char *route, const Handler &handler)
    {
        blue_print_.POST(route, handler);
    }

    void POST(const char *route, int compute_queue_id, const Handler &handler)
    {
        blue_print_.POST(route, compute_queue_id, handler);
    }

public:
    template <typename... AP>
    void GET(const char *route, const Handler &handler, const AP &...ap)
    {
        blue_print_.GET(route, handler, ap...);
    }

    template <typename... AP>
    void GET(const char *route, int compute_queue_id, 
            const Handler &handler, const AP &...ap)
    {
        blue_print_.GET(route, compute_queue_id, handler, ap...);
    }

    template <typename... AP>
    void POST(const char *route, const Handler &handler, const AP &...ap)
    {
        blue_print_.POST(route, handler, ap...);
    }

    template <typename... AP>
    void POST(const char *route, int compute_queue_id, 
            const Handler &handler, const AP &...ap)
    {
        blue_print_.POST(route, compute_queue_id, handler, ap...);
    }

public:
    void GET(const char *route, const SeriesHandler &handler)
    {
        blue_print_.GET(route, handler);
    }

    void GET(const char *route, int compute_queue_id, const SeriesHandler &handler)
    {
        blue_print_.GET(route, compute_queue_id, handler);
    }

    void POST(const char *route, const SeriesHandler &handler)
    {
        blue_print_.POST(route, handler);
    }

    void POST(const char *route, int compute_queue_id, const SeriesHandler &handler)
    {
        blue_print_.POST(route, compute_queue_id, handler);
    }

public:
    template <typename... AP>
    void GET(const char *route, const SeriesHandler &handler, const AP &...ap)
    {
        blue_print_.GET(route, handler, ap...);
    }

    template <typename... AP>
    void GET(const char *route, int compute_queue_id, 
            const SeriesHandler &handler, const AP &...ap)
    {
        blue_print_.GET(route, compute_queue_id, handler, ap...);
    }

    template <typename... AP>
    void POST(const char *route, const SeriesHandler &handler, const AP &...ap)
    {
        blue_print_.POST(route, handler, ap...);
    }

    template <typename... AP>
    void POST(const char *route, int compute_queue_id, 
            const SeriesHandler &handler, const AP &...ap)
    {
        blue_print_.POST(route, compute_queue_id, handler, ap...);
    }

public:
    void Static(const char *relative_path, const char *root);

    void list_routes();

    void register_blueprint(const BluePrint &bp, const std::string &url_prefix);

public:
    HttpServer() :
            WFServer(std::bind(&HttpServer::process, this, std::placeholders::_1))
    {}

    HttpServer &max_connections(size_t max_connections)
    {
        this->params.max_connections = max_connections;
        return *this;
    }

    HttpServer &peer_response_timeout(int peer_response_timeout)
    {
        this->params.peer_response_timeout = peer_response_timeout;
        return *this;
    }

    HttpServer &receive_timeout(int receive_timeout)
    {
        this->params.receive_timeout = receive_timeout;
        return *this;
    }

    HttpServer &keep_alive_timeout(int keep_alive_timeout)
    {
        this->params.keep_alive_timeout = keep_alive_timeout;
        return *this;
    }

    HttpServer &request_size_limit(size_t request_size_limit)
    {
        this->params.request_size_limit = request_size_limit;
        return *this;
    }

    HttpServer &ssl_accept_timeout(int ssl_accept_timeout)
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