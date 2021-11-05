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
        std::string Body() const;

        void test()
        { fprintf(stderr, "req test : %s\n", get_request_uri()); }

        void set_route_params(RouteParams &&params)
        {
            route_params = std::move(params);
        }

        std::string param(const std::string &key)
        { return route_params[key]; };


        template <typename T>
        T param(const std::string& key) const;
    private:
        RouteParams route_params;
    };

    template<>
    inline int HttpReq::param<int>(const std::string& key) const
    {
        return std::stoi(route_params.at(key));
    }

    template<>
    inline size_t HttpReq::param<size_t>(const std::string& key) const
    {
        return static_cast<size_t>(std::stoul(route_params.at(key)));
    }

    template<>
    inline double HttpReq::param<double>(const std::string& key) const
    {
        return std::stod(route_params.at(key));
    }


    class HttpResp : public protocol::HttpResponse
    {
    public:
        void String(const std::string &str);
        void String(std::string &&str);

        void String(const char *data, size_t len);

        void File(const std::string &path);

        // void Write(const std::string& content, const std::string& path);

        void set_status(int status_code);

        void test()
        { fprintf(stderr, "resp test : %s\n", get_status_code()); }
    };

    using WebTask = WFNetworkTask<HttpReq, HttpResp>;

} // namespace wfrest


#endif // _HTTPMSG_H_