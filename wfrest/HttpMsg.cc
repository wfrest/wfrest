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
    std::string content_type_str = header("Content-Type");
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
        multi_part_.set_boundary(std::move(boundary_str.as_string()));
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
    if (headers_.find("Content-Encoding") != headers_.end())
    {
        if (headers_["Content-Encoding"].find("gzip") != std::string::npos)
        {
            compress_data = Compressor::gzip(str.c_str(), str.size());
        } else if (headers_["Content-Encoding"].find("br") != std::string::npos)
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
    headers_["Content-Type"] = "multipart/form-data";
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
    headers_["Content-Type"] = "application/json";
    this->String(json.dump());
}

void HttpResp::Json(const std::string &str)
{
    if (!Json::accept(str))
    {
        this->String("JSON is invalid");
        return;
    }
    this->headers_["Content-Type"] = "application/json";
    // todo : should we just don't care format?
    // this->String(str);
    this->String(Json::parse(str).dump());
}

void HttpResp::set_compress(const enum Compress &compress)
{
    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Encoding
    headers_["Content-Encoding"] = compress_method_to_str(compress);
}



