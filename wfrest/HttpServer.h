#ifndef WFREST_HTTPSERVER_H_
#define WFREST_HTTPSERVER_H_

#include "workflow/WFHttpServer.h"

#include <unordered_map>
#include <string>

#include "wfrest/HttpMsg.h"
#include "wfrest/Router.h"
#include "wfrest/VerbHandler.h"

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
    void GET(const char *route, const SeriesHandler &series_handler);

    void GET(const char *route, int compute_queue_id, const SeriesHandler &series_handler);

    void POST(const char *route, const SeriesHandler &series_handler);

    void POST(const char *route, int compute_queue_id, const SeriesHandler &series_handler);

    void mount(std::string &&path);

	int start(unsigned short port)
	{
        router_.print_routes();
        return WFServerBase::start(port);
	}

protected:
    CommSession *new_session(long long seq, CommConnection *conn) override;

private:
    void process(HttpTask *task);

private:
    Router router_;
};

}  // namespace wfrest

#endif // WFREST_HTTPSERVER_H_