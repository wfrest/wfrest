#include "HttpMsg.h"
#include "workflow/HttpUtil.h"
#include <unistd.h>
#include <cstdlib>
#include <fcntl.h>
#include "UriUtil.h"
#include "StrUtil.h"
#include "Global.h"

using namespace wfrest;

namespace
{

    struct save_context
    {
        std::string content;
        save_context* next = nullptr;
    };

    void save_callback(HttpTask *server_task)
    {
        auto *ctx = static_cast<save_context *>(server_task->user_data);
        while(ctx != nullptr)
        {
            auto *ctx_tmp = ctx;
            ctx = ctx->next;
            delete ctx_tmp;
        }
    }

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
    StringPiece body_str(body, len);

    if (body_str.empty()) return;

    fill_content_type();

    switch (content_type)
    {
        case X_WWW_FORM_URLENCODED:
            Urlencode::parse_query_params(body_str, kv);
            break;
        case MULTIPART_FORM_DATA:
            multi_part_.parse_multipart(body_str, form);
            break; // do nothing
        default:
            break;// do nothing
    }
}

void HttpReq::fill_content_type()
{
    std::string content_type_str = header("Content-Type");
    content_type = ContentType::to_enum(content_type_str);

//    if (content_type == CONTENT_TYPE_NONE) {
//        if (!form.empty()) {
//            content_type = MULTIPART_FORM_DATA;
//        }
//        else if (!kv.empty()) {
//            content_type = X_WWW_FORM_URLENCODED;
//        }
//        else {
//            content_type = TEXT_PLAIN;
//        }
//    }
    // todo : we need fill this in header? add interface to change the header value?
    // if (content_type != CONTENT_TYPE_NONE) {
    //    header("Content-Type") = ContentType::to_string(content_type);
    // }

    if (content_type == MULTIPART_FORM_DATA)
    {
        // if type is multipart form, we reserve the boudary first
        const char *boundary = strstr(content_type_str.c_str(), "boundary=");
        if (boundary == nullptr)
        {
            // todo : do we need to add default to header field ?
            // header("Content-Type") += "; boundary=" + MultiPartForm::default_boundary;
            // multi_part_.set_boundary(MultiPartForm::default_boundary);
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
    for(auto& part : form)
    {
        if(part.second.is_file())
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

void HttpResp::File(const std::string &path, size_t start, size_t end)
{
    Global::get_http_file()->send_file(path, start, end, this);
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
    if(server_task_->user_data == nullptr)
    {
        server_task_->user_data = ctx;
        server_task_->add_callback(save_callback);
    } else
    {
        ctx->next = static_cast<save_context *>(server_task_->user_data);
        server_task_->user_data = ctx;
    }
}

void HttpResp::Save(const std::string &file_dst, const char *content, size_t len)
{
    Save(file_dst, static_cast<const void *>(content), len);
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
    if(server_task_->user_data == nullptr)
    {
        server_task_->user_data = ctx;
        server_task_->add_callback(save_callback);
    } else
    {
        ctx->next = static_cast<save_context *>(server_task_->user_data);
        server_task_->user_data = ctx;
    }

}




