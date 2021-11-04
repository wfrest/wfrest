#ifndef _HTTPMSG_H_
#define _HTTPMSG_H_

#include "workflow/HttpMessage.h"
#include "workflow/WFTaskFactory.h"
#include <fcntl.h>
#include <unordered_map>

namespace wfrest
{
    using RouteParams = std::unordered_map<std::string, std::string>;

    class HttpReq : public protocol::HttpRequest
    {
    public:
        std::string body() const;

        void test()
        { fprintf(stderr, "req test : %s\n", get_request_uri()); }

        void set_route_params(RouteParams &&params)
        {
            route_params = std::move(params);
        }

        std::string get(const std::string &key)
        { return route_params[key]; };
    private:
        RouteParams route_params;
    };

    class HttpResp : public protocol::HttpResponse
    {
    public:
        void send(const std::string &str);

        void send(const char *data, size_t len);

        void send_no_copy(const char *data, size_t len);

        void file(const std::string &path);

        // void Write(const std::string& content, const std::string& path);

        void set_status(int status_code);

        void test()
        { fprintf(stderr, "resp test : %s\n", get_status_code()); }
    };

    using WebTask = WFNetworkTask<HttpReq, HttpResp>;

} // namespace wfrest


#endif // _HTTPMSG_H_