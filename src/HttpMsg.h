#ifndef _HTTPMSG_H_
#define _HTTPMSG_H_

#include "workflow/HttpMessage.h"
#include "workflow/WFTaskFactory.h"
#include <fcntl.h>
#include <unordered_map>
#include <workflow/HttpUtil.h>
#include "HttpDef.h"
#include "HttpContent.h"
#include "HttpFile.h"

namespace wfrest
{

    using RouteParams = std::unordered_map<std::string, std::string>;
    using QueryParams = std::unordered_map<std::string, std::string>;

    class HttpReq;
    class HttpResp;
    using WebTask = WFNetworkTask<HttpReq, HttpResp>;

    class HttpReq : public protocol::HttpRequest
    {
    public:
        ~HttpReq() override { delete header_; }
        // chunked body
        std::string Body() const;

        // body -> structured content
        void parse_body();

        // header map
        void set_header_map(protocol::HttpHeaderMap* header) { header_ = header; }
        std::string header(const std::string& key) { return header_->get(key); }
        bool has_header(const std::string& key) { return header_->key_exists(key); }

        // /{name}/{id} parans in route
        void set_route_params(RouteParams &&params)
        { route_params_ = std::move(params); }

        std::string param(const std::string &key)
        { return route_params_[key]; };

        // handler define path
        std::string full_path() const
        { return route_full_path_; }

        void set_full_path(const std::string &route_full_path)
        { route_full_path_ = route_full_path; }

        // parser uri
        void set_parsed_uri(ParsedURI &&parsed_uri)
        { parsed_uri_ = std::move(parsed_uri); }

        // url query params
        void set_query_params(QueryParams &&query_params)
        { query_params_ = std::move(query_params); }

        std::string query(const std::string &key)
        { return query_params_[key]; }

        std::string default_query(const std::string &key, const std::string &default_val);

        template<typename T>
        T param(const std::string &key) const;

        QueryParams query_list() const
        { return query_params_; }

        bool has_query(const std::string &key);

        // connect to server_task
        void set_task(WebTask *task) { server_task_ = task; };
        WebTask *get_task() const { return server_task_; }

    private:
        void fill_content_type();

    public:
        Urlencode::KV kv;
        MultiPartForm::MultiPart form;
        http_content_type content_type;
    private:
        RouteParams route_params_;
        std::string route_full_path_;
        ParsedURI parsed_uri_;
        QueryParams query_params_;
        MultiPartForm multi_part_;
        protocol::HttpHeaderMap *header_;
        WebTask* server_task_ = nullptr;
    };

    template<>
    inline int HttpReq::param<int>(const std::string &key) const
    {
        if (route_params_.count(key))
            return std::stoi(route_params_.at(key));
        else
            return 0;
    }

    template<>
    inline size_t HttpReq::param<size_t>(const std::string &key) const
    {
        if (route_params_.count(key))
            return static_cast<size_t>(std::stoul(route_params_.at(key)));
        else
            return 0;
    }

    template<>
    inline double HttpReq::param<double>(const std::string &key) const
    {
        if (route_params_.count(key))
            return std::stod(route_params_.at(key));
        else
            return 0.0;
    }


    class HttpResp : public protocol::HttpResponse
    {
    public:
        // send string
        void String(const std::string &str);
        void String(const char *data, size_t len);

        // todo : json / file clear_output_body
        // file
        void File(const std::string &path, size_t start = 0, size_t end = 0);
        // save file
        void Save(const std::string& file_dst, const char *content, size_t len);
        void Save(const std::string& file_dst, const void *content, size_t len);
        void Save(const std::string& file_dst, const std::string& content);
        void Save(const std::string& file_dst, std::string&& content);

        void set_status(int status_code);

        void test()
        { fprintf(stderr, "resp test : %s\n", get_status_code()); }

        // connect to server_task
        void set_task(WebTask *task) { server_task_ = task; };
        WebTask *get_task() const { return server_task_; }

    private:
        WebTask *server_task_ = nullptr;
    };



} // namespace wfrest


#endif // _HTTPMSG_H_