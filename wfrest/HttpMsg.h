#ifndef WFREST_HTTPMSG_H_
#define WFREST_HTTPMSG_H_

#include "workflow/HttpMessage.h"
#include "workflow/WFTaskFactory.h"

#include <fcntl.h>
#include <unordered_map>
#include <memory>

#include "wfrest/StringPiece.h"
#include "wfrest/HttpDef.h"
#include "wfrest/HttpContent.h"
#include "wfrest/Compress.h"
#include "wfrest/json_fwd.hpp"
#include "wfrest/StrUtil.h"
#include "wfrest/HttpCookie.h"
#include "wfrest/Noncopyable.h"

namespace wfrest
{

using Json = nlohmann::json;

struct ReqData;

class HttpReq : public protocol::HttpRequest, public Noncopyable
{
public:
    std::string &body() const;

    // post body
    std::map<std::string, std::string> &form_kv() const;

    Form &form() const;

    Json &json() const;

    http_content_type content_type() const
    { return content_type_; }

    const std::string &header(const std::string &key) const;

    bool has_header(const std::string &key) const;

    const std::string &param(const std::string &key) const;

    template<typename T>
    T param(const std::string &key) const;

    bool has_param(const std::string &key) const;

    const std::string &query(const std::string &key) const;

    const std::string &default_query(const std::string &key,
                                     const std::string &default_val) const;

    const std::map<std::string, std::string> &query_list() const
    { return query_params_; }

    bool has_query(const std::string &key) const;

    const std::string &match_path() const
    { return route_match_path_; }

    // handler define path
    const std::string &full_path() const
    { return route_full_path_; }

    std::string current_path() const
    { return parsed_uri_.path; }

    const std::map<std::string, std::string> &cookies() const;

    const std::string &cookie(const std::string &key) const;
public:
    void fill_content_type();

    void fill_header_map();

    // /{name}/{id} params in route
    void set_route_params(std::map<std::string, std::string> &&params)
    { route_params_ = std::move(params); }

    // /match*  
    // /match123 -> match123
    void set_route_match_path(const std::string &match_path)
    { route_match_path_ = match_path; }

    void set_full_path(const std::string &route_full_path)
    { route_full_path_ = route_full_path; }

    void set_query_params(std::map<std::string, std::string> &&query_params)
    { query_params_ = std::move(query_params); }

    void set_parsed_uri(ParsedURI &&parsed_uri)
    { parsed_uri_ = std::move(parsed_uri); }

public:
    HttpReq();

    ~HttpReq();

private:
    using HeaderMap = std::map<std::string, std::vector<std::string>, MapStringCaseLess>;
    
    http_content_type content_type_;
    ReqData *req_data_;

    std::string route_match_path_;
    std::string route_full_path_;

    std::map<std::string, std::string> route_params_;
    std::map<std::string, std::string> query_params_;
    mutable std::map<std::string, std::string> cookies_;

    MultiPartForm multi_part_;
    HeaderMap headers_;

    ParsedURI parsed_uri_;
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

class HttpResp : public protocol::HttpResponse, public Noncopyable
{
public:
    // send string
    void String(const std::string &str);

    void String(std::string &&str);

    // file
    void File(const std::string &path);

    void File(const std::string &path, size_t start);

    void File(const std::string &path, size_t start, size_t end);

    // send multiple files in multipart/form-data format
    void File(const std::vector<std::string> &path_list);

    // save file
    void Save(const std::string &file_dst, const std::string &content);

    void Save(const std::string &file_dst, std::string &&content);

    // json
    void Json(const Json &json);

    void Json(const std::string &str);

    void set_status(int status_code);

    // Compress
    void set_compress(const Compress &compress);

    void add_cookie(HttpCookie &&cookie)
    { cookies_.emplace_back(std::move(cookie)); }

    void add_cookie(const HttpCookie &cookie)
    { cookies_.push_back(cookie); }

    int get_state() const; 

    int get_error() const;
private:
    std::string compress(const std::string &str);

public:
    std::map<std::string, std::string, MapStringCaseLess> headers;
    std::vector<HttpCookie> cookies_;
    void *user_data;
};

using HttpTask = WFNetworkTask<HttpReq, HttpResp>;

} // namespace wfrest


#endif // WFREST_HTTPMSG_H_