#include "workflow/HttpUtil.h"

#include <unistd.h>
#include <algorithm>

#include "wfrest/HttpMsg.h"
#include "wfrest/UriUtil.h"
#include "wfrest/PathUtil.h"
#include "wfrest/Logger.h"
#include "wfrest/HttpFile.h"
#include "wfrest/json.hpp"
#include "wfrest/HttpServerTask.h"

using namespace wfrest;
using namespace protocol;
using Json = nlohmann::json;

namespace wfrest
{

struct ReqData
{
    std::string body;
    std::map<std::string, std::string> form_kv;
    Form form;
    Json json;
};

struct ProxyCtx
{
    std::string url;
    HttpServerTask *server_task;
    bool is_keep_alive;
};

void proxy_http_callback(WFHttpTask *http_task)
{   
    int state = http_task->get_state();
    int error = http_task->get_error();
    auto *proxy_ctx = static_cast<ProxyCtx *>(http_task->user_data);
    HttpServerTask *server_task = proxy_ctx->server_task;
    HttpResponse *http_resp = http_task->get_resp();
    HttpResp *server_resp = server_task->get_resp();
    /* Some servers may close the socket as the end of http response. */
    if (state == WFT_STATE_SYS_ERROR && error == ECONNRESET)
        state = WFT_STATE_SUCCESS;

    if (state == WFT_STATE_SUCCESS)
    {
        /* add a callback for getting reply status. */
        server_task->add_callback([proxy_ctx](HttpTask *server_task)
        {
            HttpResp *server_resp = server_task->get_resp();
            size_t size = server_resp->get_output_body_size();
            if (server_task->get_state() == WFT_STATE_SUCCESS)
                fprintf(stderr, "%s: Success. Http Status: %s, BodyLength: %zu\n",
                        proxy_ctx->url.c_str(), server_resp->get_status_code(), size);
            else /* WFT_STATE_SYS_ERROR*/
                fprintf(stderr, "%s: Reply failed: %s, BodyLength: %zu\n",
                        proxy_ctx->url.c_str(), strerror(server_task->get_error()), size);

            delete proxy_ctx;
        });

        const void *body;
        size_t len;
        // Copy the remote webserver's response, to server response.
        if (http_resp->get_parsed_body(&body, &len))
            http_resp->append_output_body_nocopy(body, len);

        HttpResp resp(std::move(*http_resp));
        *server_resp = std::move(resp);
        
        if (!proxy_ctx->is_keep_alive)
            server_resp->set_header_pair("Connection", "close");
    }
    else
    {
        fprintf(stderr, "555");
        const char *err_string;
        int error = http_task->get_error();

        if (state == WFT_STATE_SYS_ERROR)
            err_string = strerror(error);
        else if (state == WFT_STATE_DNS_ERROR)
            err_string = gai_strerror(error);
        else if (state == WFT_STATE_SSL_ERROR)
            err_string = "SSL error";
        else /* if (state == WFT_STATE_TASK_ERROR) */
            err_string = "URL error (Cannot be a HTTPS proxy)";

        fprintf(stderr, "%s: Fetch failed. state = %d, error = %d: %s\n",
                proxy_ctx->url.c_str(), state, http_task->get_error(),
                err_string);

        server_resp->set_status_code("404");
        server_resp->append_output_body_nocopy("<html>404 Not Found.</html>", 27);
    }
}

} // namespace wfrest


HttpReq::HttpReq() : req_data_(new ReqData)
{}

HttpReq::~HttpReq()
{
    delete req_data_;
}

std::string &HttpReq::body() const
{
    if (req_data_->body.empty())
    {
        std::string content = protocol::HttpUtil::decode_chunked_body(this);

        std::string header = this->header("Content-Encoding");
        if (header.find("gzip") != std::string::npos)
        {
            LOG_DEBUG << "Ungzip ReqData";
            req_data_->body = Compressor::ungzip(content.c_str(), content.size());
        } else if (header.find("br") != std::string::npos)
        {
            LOG_DEBUG << "UnBrotli ReqData";
            // not implement yet
            req_data_->body = Compressor::unbrotli(content.c_str(), content.size());
        } else
        {
            req_data_->body = std::move(content);
        }
    }
    return req_data_->body;
}

std::map<std::string, std::string> &HttpReq::form_kv() const
{
    if (content_type_ == APPLICATION_URLENCODED && req_data_->form_kv.empty())
    {
        StringPiece body_piece(this->body());
        req_data_->form_kv = Urlencode::parse_post_kv(body_piece);
    }
    return req_data_->form_kv;
}

Form &HttpReq::form() const
{
    if (content_type_ == MULTIPART_FORM_DATA && req_data_->form.empty())
    {
        StringPiece body_piece(this->body());

        req_data_->form = multi_part_.parse_multipart(body_piece);
    }
    return req_data_->form;
}

Json &HttpReq::json() const
{
    if (content_type_ == APPLICATION_JSON && req_data_->json.empty())
    {
        const std::string &body_content = this->body();
        if (!Json::accept(body_content))
        {
            LOG_ERROR << "Json is invalid";
            return req_data_->json;
            // todo : how to let user know the error ?
        }
        req_data_->json = Json::parse(body_content);
    }
    return req_data_->json;
}

const std::string &HttpReq::param(const std::string &key) const
{
    if (route_params_.count(key))
        return route_params_.at(key);
    else
        return string_not_found;
}

bool HttpReq::has_param(const std::string &key) const
{
    return route_params_.count(key) > 0;
}

const std::string &HttpReq::query(const std::string &key) const
{
    if (query_params_.count(key))
        return query_params_.at(key);
    else
        return string_not_found;
}

const std::string &HttpReq::default_query(const std::string &key, const std::string &default_val) const
{
    if (query_params_.count(key))
        return query_params_.at(key);
    else
        return default_val;
}

bool HttpReq::has_query(const std::string &key) const
{
    if (query_params_.find(key) != query_params_.end())
    {
        return true;
    } else
    {
        return false;
    }
}

void HttpReq::fill_content_type()
{
    const std::string &content_type_str = header("Content-Type");
    content_type_ = ContentType::to_enum(content_type_str);

    if (content_type_ == MULTIPART_FORM_DATA)
    {
        // if type is multipart form, we reserve the boudary first
        const char *boundary = strstr(content_type_str.c_str(), "boundary=");
        if (boundary == nullptr)
        {
            return;
        }
        boundary += strlen("boundary=");
        StringPiece boundary_piece(boundary);

        StringPiece boundary_str = StrUtil::trim_pairs(boundary_piece, R"(""'')");
        multi_part_.set_boundary(boundary_str.as_string());
    }
}

const std::string &HttpReq::header(const std::string &key) const
{
    const auto it = headers_.find(key);

    if (it == headers_.end() || it->second.empty())
        return string_not_found;

    return it->second[0];
}

bool HttpReq::has_header(const std::string &key) const
{
    return headers_.count(key) > 0;
}

void HttpReq::fill_header_map()
{
    http_header_cursor_t cursor;
    struct protocol::HttpMessageHeader header;

    http_header_cursor_init(&cursor, this->get_parser());
    while (http_header_cursor_next(&header.name, &header.name_len,
                                   &header.value, &header.value_len,
                                   &cursor) == 0)
    {
        std::string key(static_cast<const char *>(header.name), header.name_len);

        headers_[key].emplace_back(static_cast<const char *>(header.value), header.value_len);
    }

    http_header_cursor_deinit(&cursor);
}

const std::map<std::string, std::string> &HttpReq::cookies() const
{
    if (cookies_.empty() && this->has_header("Cookie"))
    {
        const std::string &cookie = this->header("Cookie");
        StringPiece cookie_piece(cookie);
        cookies_ = std::move(HttpCookie::split(cookie_piece));
    }
    return cookies_;
}

const std::string &HttpReq::cookie(const std::string &key) const
{
    if(cookies_.empty()) 
    {
        this->cookies();
    }
    if(cookies_.find(key) != cookies_.end())
    {
        return cookies_[key];
    }
    return string_not_found;
}

HttpReq::HttpReq(HttpReq&& other)
    : HttpRequest(std::move(other)),
    content_type_(other.content_type_),
    route_match_path_(std::move(other.route_match_path_)),
    route_full_path_(std::move(other.route_full_path_)),
    route_params_(std::move(other.route_params_)),
    query_params_(std::move(other.query_params_)),
    cookies_(std::move(other.cookies_)),
    multi_part_(std::move(other.multi_part_)),
    headers_(std::move(other.headers_)),
    parsed_uri_(std::move(other.parsed_uri_))
{
    req_data_ = other.req_data_;
    other.req_data_ = nullptr;
}

HttpReq &HttpReq::operator=(HttpReq&& other)
{
    HttpRequest::operator=(std::move(other));
    content_type_ = other.content_type_;

    req_data_ = other.req_data_;
    other.req_data_ = nullptr;

    route_match_path_ = std::move(other.route_match_path_);
    route_full_path_ = std::move(other.route_full_path_);
    route_params_ = std::move(other.route_params_);
    query_params_ = std::move(other.query_params_);
    cookies_ = std::move(other.cookies_);
    multi_part_ = std::move(other.multi_part_);
    headers_ = std::move(other.headers_);
    parsed_uri_ = std::move(other.parsed_uri_);

    return *this;
}

void HttpResp::String(const std::string &str)
{
    std::string compres_data = this->compress(str);
    if (compres_data.empty())
    {
        this->append_output_body(static_cast<const void *>(str.c_str()), str.size());
    } else
    {
        this->append_output_body(static_cast<const void *>(compres_data.c_str()), compres_data.size());
    }
}

void HttpResp::String(std::string &&str)
{
    std::string compres_data = this->compress(str);
    std::string *data = new std::string;
    if (compres_data.empty())
    {
        *data = std::move(str);
    } else
    {
        *data = std::move(compres_data);
    }
    this->append_output_body_nocopy(data->c_str(), data->size());
    task_of(this)->add_callback([data](HttpTask *) { delete data; });
}

std::string HttpResp::compress(const std::string &str)
{
    std::string compress_data;
    if (headers.find("Content-Encoding") != headers.end())
    {
        if (headers["Content-Encoding"].find("gzip") != std::string::npos)
        {
            compress_data = Compressor::gzip(str.c_str(), str.size());
        } else if (headers["Content-Encoding"].find("br") != std::string::npos)
        {
            compress_data = Compressor::brotli(str.c_str(), str.size());
        }
    }
    return compress_data;
}

void HttpResp::File(const std::string &path)
{
    HttpFile::send_file(path, 0, -1, this);
}

void HttpResp::File(const std::string &path, size_t start)
{
    HttpFile::send_file(path, start, -1, this);
}

void HttpResp::File(const std::string &path, size_t start, size_t end)
{
    HttpFile::send_file(path, start, end, this);
}

void HttpResp::File(const std::vector<std::string> &path_list)
{
    headers["Content-Type"] = "multipart/form-data";
    for (int i = 0; i < path_list.size(); i++)
    {
        HttpFile::send_file_for_multi(path_list, i, this);
    }
}

void HttpResp::set_status(int status_code)
{
    protocol::HttpUtil::set_response_status(this, status_code);
}

void HttpResp::Save(const std::string &file_dst, const std::string &content)
{
    HttpFile::save_file(file_dst, content, this);
}

void HttpResp::Save(const std::string &file_dst, std::string &&content)
{
    HttpFile::save_file(file_dst, std::move(content), this);
}

void HttpResp::Json(const ::Json &json)
{
    // The header value itself does not allow for multiple values, 
    // and it is also not allowed to send multiple Content-Type headers
    // https://stackoverflow.com/questions/5809099/does-the-http-protocol-support-multiple-content-types-in-response-headers
    headers["Content-Type"] = "application/json";
    this->String(json.dump());
}

void HttpResp::Json(const std::string &str)
{
    if (!Json::accept(str))
    {
        this->String("JSON is invalid");
        return;
    }
    this->headers["Content-Type"] = "application/json";
    // todo : should we just don't care format?
    // this->String(str);
    this->String(Json::parse(str).dump());
}

void HttpResp::set_compress(const enum Compress &compress)
{
    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Encoding
    headers["Content-Encoding"] = compress_method_to_str(compress);
}

int HttpResp::get_state() const
{
    HttpServerTask *server_task = task_of(this);
    return server_task->get_state();   
}

int HttpResp::get_error() const
{
    HttpServerTask *server_task = task_of(this);
    return server_task->get_error();   
}

void HttpResp::Http(const std::string &url, size_t limit_size)
{
    HttpServerTask *server_task = task_of(this);
    HttpReq *server_req = server_task->get_req();

    // for requesting remote webserver. 
    WFHttpTask *http_task = WFTaskFactory::create_http_task(url, 
                                                            0, 
                                                            0, 
                                                            proxy_http_callback);

    auto *proxy_ctx = new ProxyCtx;
    proxy_ctx->url = url;
    proxy_ctx->server_task = server_task;
    proxy_ctx->is_keep_alive = server_req->is_keep_alive();
    http_task->user_data = proxy_ctx;

    const void *body;
	size_t len;             

	// Copy user's request to the new task's reuqest using std::move() 
	server_req->set_request_uri(url);
	server_req->get_parsed_body(&body, &len);
	server_req->append_output_body_nocopy(body, len);
	*http_task->get_req() = std::move(*server_req);  

	// also, limit the remote webserver response size. 
	http_task->get_resp()->set_size_limit(limit_size);

	**server_task << http_task;
}

HttpResp::HttpResp(HttpResp&& other)
    : HttpResponse(std::move(other)),
    headers(std::move(other.headers)),
    cookies_(std::move(other.cookies_))
{
    user_data = other.user_data;
    other.user_data = nullptr;
}

HttpResp &HttpResp::operator=(HttpResp&& other)
{
    HttpResponse::operator=(std::move(other));
    headers = std::move(other.headers);
    user_data = other.user_data;
    other.user_data = nullptr;
    cookies_ = std::move(other.cookies_);
    return *this;
}