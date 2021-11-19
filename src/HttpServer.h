#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include <workflow/WFHttpServer.h>

#include <unordered_map>
#include <string>

#include "HttpMsg.h"
#include "Router.h"
#include "VerbHandler.h"

namespace wfrest
{
class HttpServer : public WFServer<HttpReq, HttpResp>
{
public:
    HttpServer() :
            WFServer(std::bind(&HttpServer::proc, this, std::placeholders::_1))
    {}

    void Get(const char *route, const Handler &handler);

    void Post(const char *route, const Handler &handler);

    void mount(std::string &&path);

protected:
    CommSession *new_session(long long seq, CommConnection *conn) override;

private:
    void proc(HttpTask *task);

private:
    Router router_{};
};

}  // namespace wfrest

#endif // _HTTPSERVER_H_