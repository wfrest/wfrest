#ifndef _WFWEBSERVER_H_
#define _WFWEBSERVER_H_

#include <workflow/WFHttpServer.h>
#include <unordered_map>
#include <string>
#include "WFHttpMsg.h"
#include "Router.h"

namespace wfrest
{

    class WFWebServer : public WFServer<HttpReq, HttpResp>
    {
    public:
        using Handler = std::function<void(const HttpReq *, HttpResp *)>;
        enum
        {
            ANY, GET, POST, PUT, HTTP_DELETE
        };
    public:
        WFWebServer() :
                WFServer(std::bind(&WFWebServer::proc, this, std::placeholders::_1))
        {}

        void Get(std::string &&route, const Handler &handler);

        void Post(std::string &&route, const Handler &handler);

        void start_ssl(bool is_ssl)
        {
            is_ssl_ = is_ssl;
        }

        static WFWebTask *task_of(const HttpReq *req);

        static WFWebTask *task_of(const HttpResp *resp);

    protected:
        CommSession *new_session(long long seq, CommConnection *conn) override;

    private:
        void proc(WFWebTask *server_task);

        void dispatch_request(HttpReq *req, HttpResp *resp) const;

    private:
        Router router_;
        bool is_ssl_ = false;
    };

}  // namespace wfrest







#endif // _WFWEBSERVER_H_