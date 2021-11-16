#include <workflow/HttpUtil.h>

#include <unistd.h>
#include <cstdlib>
#include <fcntl.h>

#include "HttpMsg.h"
#include "UriUtil.h"
#include "StrUtil.h"
#include "Global.h"
#include "PathUtil.h"

using namespace wfrest;

namespace
{
    struct save_context
    {
        std::string content;
    };

} // namespace


std::string HttpReq::Body() const
{
    return protocol::HttpUtil::decode_chunked_body(this);
}

std::string HttpReq::default_query(const std::string &key, const std::string &default_val)
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

void HttpReq::parse_body()
{
    const void *body;
    size_t len;
    this->get_parsed_body(&body, &len);

    StringPiece body_piece(body, len);
    if (body_piece.empty()) return;

    fill_content_type();

    std::string chunked_body;
    if ((this->is_chunked() && (content_type == X_WWW_FORM_URLENCODED || content_type == MULTIPART_FORM_DATA))
        || content_type == APPLICATION_JSON)
    {
        chunked_body = this->Body();
        body_piece = StringPiece(chunked_body);
    }

    switch (content_type)
    {
        case X_WWW_FORM_URLENCODED:
        {
            Urlencode::parse_query_params(body_piece, kv);
            break;
        }
        case MULTIPART_FORM_DATA:
        {
            multi_part_.parse_multipart(body_piece, form);
            break;
        }
        case APPLICATION_JSON:
        {
            fprintf(stderr, "%s\n", chunked_body.c_str());
            if (!Json::accept(chunked_body))
            {
                fprintf(stderr, "json is invalid\n");
                break;
                // todo : how to let user know the error ?
            }
            json = Json::parse(chunked_body);
        }
        default:
            break;// do nothing
    }
}

void HttpReq::fill_content_type()
{
    std::string content_type_str = header("Content-Type");
    content_type = ContentType::to_enum(content_type_str);

    if (content_type == MULTIPART_FORM_DATA)
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

FormData *HttpReq::post_form(const std::string &key)
{
    if (form.find(key) != form.end()) return &form[key];
    else return nullptr;
}

std::vector<FormData *> HttpReq::post_files()
{
    std::vector<FormData *> res;
    for (auto &part: form)
    {
        if (part.second.is_file())
        {
            res.push_back(&part.second);
        }
    }
    return res;
}

void HttpResp::String(const std::string &str)
{
    // bool append_output_body(const void *buf, size_t size);
    this->append_output_body(static_cast<const void *>(str.c_str()), str.size());
}

void HttpResp::String(const char *data, size_t len)
{
    this->append_output_body(static_cast<const void *>(data), len);
}

void HttpResp::File(const std::string &path, int start, int end)
{
    Global::get_http_file()->send_file(path, start, end, this);
}

void HttpResp::File(const std::vector<std::string> &path_list)
{
    this->add_header_pair("Content-Type", "multipart/form-data");
    for (int i = 0; i < path_list.size(); i++)
    {
        Global::get_http_file()->send_file_for_multi(path_list, i, this);
    }
}

void HttpResp::set_status(int status_code)
{
    protocol::HttpUtil::set_response_status(this, status_code);
}

void HttpResp::Save(const std::string &file_dst, const void *content, size_t len)
{
    auto *ctx = new save_context;
    ctx->content = std::string(static_cast<const char *>(content), len);
    Global::get_http_file()->save_file(file_dst, content, len, this);
    server_task_->add_callback([ctx](const HttpTask *)
                               {
                                   delete ctx;
                               });
}

void HttpResp::Save(const std::string &file_dst, const std::string &content)
{
    Save(file_dst, static_cast<const void *>(content.c_str()), content.size());
}

void HttpResp::Save(const std::string &file_dst, std::string &&content)
{
    auto *ctx = new save_context;
    ctx->content = std::move(content);
    Global::get_http_file()->save_file(file_dst,
                                       static_cast<const void *>(ctx->content.c_str()),
                                       ctx->content.size(),
                                       this);
    server_task_->add_callback([ctx](const HttpTask *)
                               {
                                   delete ctx;
                               });
}

void HttpResp::Json(const ::Json &json)
{
    this->add_header_pair("Content-Type", "application/json");
    this->String(json.dump());
}

void HttpResp::Json(const std::string &str)
{
    if (!Json::accept(str))
    {
        this->String("JSON is invalid");
        return;
    }
    this->add_header_pair("Content-Type", "application/json");
    // should we just don't care format?
    // this->String(str);
    this->String(Json::parse(str).dump());
}



