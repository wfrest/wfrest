#include "workflow/HttpUtil.h"

#include <unistd.h>

#include "wfrest/HttpMsg.h"
#include "wfrest/UriUtil.h"
#include "wfrest/StrUtil.h"
#include "wfrest/PathUtil.h"
#include "wfrest/Logger.h"
#include "wfrest/HttpFile.h"
#include "wfrest/json.hpp"

using Json = nlohmann::json;

namespace wfrest
{

struct Body
{
    std::string content;
    std::map<std::string, std::string> form_kv;
    Form form;
    Json json;
};

} // namespace wfrest

using namespace wfrest;

HttpReq::HttpReq() : body_(new Body) {}

HttpReq::~HttpReq()
{
    delete body_;
}

std::string& HttpReq::body()
{
    if(body_->content.empty())
    {
        body_->content = protocol::HttpUtil::decode_chunked_body(this);
    }
    return body_->content;
}

std::map<std::string, std::string> &HttpReq::form_kv()
{
    if(content_type_ == APPLICATION_URLENCODED && body_->form_kv.empty())
    {
        StringPiece body_piece(this->body());
        body_->form_kv = Urlencode::parse_post_kv(body_piece);
    }
    return body_->form_kv;
}

Form &HttpReq::form()
{
    if(content_type_ == MULTIPART_FORM_DATA && body_->form.empty())
    {
        StringPiece body_piece(this->body());
        body_->form = multi_part_.parse_multipart(body_piece);
    }
    return body_->form;
}

Json &HttpReq::json()
{
    if(content_type_ == APPLICATION_JSON && body_->json.empty())
    {
        const std::string& body_content = this->body();
        if (!Json::accept(body_content))
        {
            LOG_ERROR << "Json is invalid";
            return body_->json;
            // todo : how to let user know the error ?
        }
        body_->json = Json::parse(body_content);
    }
    return body_->json;
}

const std::string &HttpReq::default_query(const std::string &key, const std::string &default_val)
{
    if (query_params_.count(key))
        return query_params_[key];
    else
        return default_val;
}

bool HttpReq::has_query(const std::string &key)
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

// std::string HttpReq::ungzip()
// {
//     std::string body = this->body();
//     return Compressor::ungzip(body.c_str(), body.size());
// }

void HttpResp::String(const std::string &str)
{
    // bool append_output_body(const void *buf, size_t size);
    this->append_output_body(static_cast<const void *>(str.c_str()), str.size());
}

void HttpResp::String(std::string &&str)
{
    this->append_output_body(static_cast<const void *>(str.c_str()), str.size());
}

 void HttpResp::String(const std::string &str, Compress compress)
 {
     // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Encoding
     this->headers["Content-Encoding"] = compress_method_to_str(compress);
     std::string compress_str;
     if(compress == Compress::GZIP)
     {
         compress_str = Compressor::gzip(str.c_str(), str.size());
     } else if(compress == Compress::BROTLI)
     {
         // not implement yet
         compress_str = std::move(str);
     } else
     {
         LOG_DEBUG << "Dosen't support this compression, No Compression...";
         compress_str = std::move(str);
     }
     this->String(std::move(compress_str));
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
    this->headers["Content-Type"] = "multipart/form-data";
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
    this->headers["Content-Type"] = "application/json";
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




