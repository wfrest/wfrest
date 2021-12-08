#ifndef WFREST_HTTPSERVER_H_
#define WFREST_HTTPSERVER_H_

#include "workflow/WFHttpServer.h"

#include <unordered_map>
#include <string>

#include "wfrest/HttpMsg.h"
#include "wfrest/BluePrint.h"

namespace wfrest
{

class HttpServer : public WFServer<HttpReq, HttpResp>
{
public:
    HttpServer() :
            WFServer(std::bind(&HttpServer::process, this, std::placeholders::_1))
    {}

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
    void mount(std::string &&path);

    void list_routes();

    void register_blueprint(const BluePrint& bp, const std::string& url_prefix);

protected:
    CommSession *new_session(long long seq, CommConnection *conn) override;

private:
    void process(HttpTask *task);

private:
    BluePrint bp_;
};

}  // namespace wfrest

#endif // WFREST_HTTPSERVER_H_