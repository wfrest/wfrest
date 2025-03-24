#include "workflow/HttpUtil.h"
#include "workflow/MySQLResult.h"
#include "workflow/WFMySQLConnection.h"
#include "workflow/Workflow.h"
#include "workflow/WFTaskFactory.h"

#include <unistd.h>
#include <algorithm>

#include "HttpMsg.h"
#include "UriUtil.h"
#include "PathUtil.h"
#include "MysqlUtil.h"
#include "ErrorCode.h"
#include "FileUtil.h"
#include "HttpServerTask.h"
#include "CodeUtil.h"

using namespace protocol;

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

    // Some servers may close the socket as the end of http response.
    if (state == WFT_STATE_SYS_ERROR && error == ECONNRESET)
        state = WFT_STATE_SUCCESS;

    if (state == WFT_STATE_SUCCESS)
    {
        server_task->add_callback([proxy_ctx](HttpTask *server_task)
        {
            HttpResp *server_resp = server_task->get_resp();
            size_t size = server_resp->get_output_body_size();
            if (server_task->get_state() != WFT_STATE_SUCCESS)
            {
                std::string errmsg;
                errmsg.reserve(64);
                errmsg.append(proxy_ctx->url);
                errmsg.append(" : Reply failed: ");
                errmsg.append(strerror(server_task->get_error()));
                errmsg.append(", BodyLength: ");
                errmsg.append(std::to_string(size));
                server_resp->Error(StatusProxyError, errmsg);
            }
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

        std::string errmsg;
        errmsg.reserve(64);
        errmsg.append(proxy_ctx->url);
        errmsg.append(" : Fetch failed. state = ");
        errmsg.append(std::to_string(state));
        errmsg.append(", error = ");
        errmsg.append(std::to_string(http_task->get_error()));
        errmsg.append(" ");
        errmsg.append(err_string);
        server_resp->Error(StatusProxyError, errmsg);
    }
    server_task->add_callback([proxy_ctx](HttpTask *server_task)
    {
        delete proxy_ctx;
    });
    // move back Request
    auto *server_req = static_cast<HttpRequest *>(server_task->get_req());
    *server_req = std::move(*http_task->get_req());
}

wfrest::Json mysql_concat_json_res(WFMySQLTask *mysql_task)
{
    wfrest::Json json;
    MySQLResponse *mysql_resp = mysql_task->get_resp();
    MySQLResultCursor cursor(mysql_resp);
    const MySQLField *const *fields;
    std::vector<MySQLCell> arr;

    if (mysql_task->get_state() != WFT_STATE_SUCCESS)
    {
        json["error"] = WFGlobal::get_error_string(mysql_task->get_state(),
                                                   mysql_task->get_error());
        return json;
    }

    do {
        wfrest::Json result_set;
        if (cursor.get_cursor_status() != MYSQL_STATUS_GET_RESULT &&
            cursor.get_cursor_status() != MYSQL_STATUS_OK)
        {
            break;
        }

        if (cursor.get_cursor_status() == MYSQL_STATUS_GET_RESULT)
        {
            result_set["field_count"] = cursor.get_field_count();
            result_set["rows_count"] = cursor.get_rows_count();
            fields = cursor.fetch_fields();
            std::vector<std::string> fields_name;
            std::vector<std::string> fields_type;
            for (int i = 0; i < cursor.get_field_count(); i++)
            {
                if (i == 0)
                {
                    std::string database = fields[i]->get_db();
                    if(!database.empty())
                        result_set["database"] = std::move(database);
                    result_set["table"] = fields[i]->get_table();
                }

                fields_name.push_back(fields[i]->get_name());
                fields_type.push_back(datatype2str(fields[i]->get_data_type()));
            }
            result_set["fields_name"] = fields_name;
            result_set["fields_type"] = fields_type;

            while (cursor.fetch_row(arr))
            {
                wfrest::Json row;
                for (size_t i = 0; i < arr.size(); i++)
                {
                    if (arr[i].is_string())
                    {
                        row.push_back(arr[i].as_string());
                    }
                    else if (arr[i].is_time() || arr[i].is_datetime())
                    {
                        row.push_back(MySQLUtil::to_string(arr[i]));
                    }
                    else if (arr[i].is_null())
                    {
                        row.push_back("NULL");
                    }
                    else if(arr[i].is_double())
                    {
                        row.push_back(arr[i].as_double());
                    }
                    else if(arr[i].is_float())
                    {
                        row.push_back(arr[i].as_float());
                    }
                    else if(arr[i].is_int())
                    {
                        row.push_back(arr[i].as_int());
                    }
                    else if(arr[i].is_ulonglong())
                    {
                        row.push_back(arr[i].as_ulonglong());
                    }
                }
                result_set["rows"].push_back(row);
            }
        }
        else if (cursor.get_cursor_status() == MYSQL_STATUS_OK)
        {
            result_set["status"] = "OK";
            result_set["affected_rows"] = cursor.get_affected_rows();
            result_set["warnings"] = cursor.get_warnings();
            result_set["insert_id"] = cursor.get_insert_id();
            result_set["info"] = cursor.get_info();
        }
        json["result_set"].push_back(result_set);
    } while (cursor.next_result_set());

    if (mysql_resp->get_packet_type() == MYSQL_PACKET_ERROR)
    {
        json["errcode"] = mysql_task->get_resp()->get_error_code();
        json["errmsg"] = mysql_task->get_resp()->get_error_msg();
    }
    else if (mysql_resp->get_packet_type() == MYSQL_PACKET_OK)
    {
        json["status"] = "OK";
        json["affected_rows"] = mysql_task->get_resp()->get_affected_rows();
        json["warnings"] = mysql_task->get_resp()->get_warnings();
        json["insert_id"] = mysql_task->get_resp()->get_last_insert_id();
        json["info"] = mysql_task->get_resp()->get_info();
    }
    return json;
}

wfrest::Json redis_json_res(WFRedisTask *redis_task)
{
    RedisRequest *redis_req = redis_task->get_req();
    RedisResponse *redis_resp = redis_task->get_resp();
    int state = redis_task->get_state();
    int error = redis_task->get_error();
    RedisValue val;
    wfrest::Json js;
    switch (state)
    {
    case WFT_STATE_SYS_ERROR:
        js["errmsg"] = "system error: " + std::string(strerror(error));
        break;
    case WFT_STATE_DNS_ERROR:
        js["errmsg"] = "DNS error: " + std::string(gai_strerror(error));
        break;
    case WFT_STATE_SSL_ERROR:
        js["errmsg"] = "SSL error: " + std::to_string(error);
        break;
    case WFT_STATE_TASK_ERROR:
        js["errmsg"] = "Task error: " + std::to_string(error);
        break;
    case WFT_STATE_SUCCESS:
        redis_resp->get_result(val);
        if (val.is_error())
        {
            js["errmsg"] = "Error reply. Need a password?\n";
            state = WFT_STATE_TASK_ERROR;
        }
        break;
    }
    std::string cmd;
    std::vector<std::string> params;
    redis_req->get_command(cmd);
    redis_req->get_params(params);
    if (state == WFT_STATE_SUCCESS && cmd == "SET")
    {
        js["status"] = "success";
        js["cmd"] = "SET";
        js[params[0]] = params[1];
    }
    if(state == WFT_STATE_SUCCESS && cmd == "GET")
    {
        js["cmd"] = "GET";
        if (val.is_string())
        {
            js[params[0]] = val.string_value();
            js["status"] = "success";
        }
        else
        {
            js["errmsg"] = "value is not a string value";
        }
    }
    return js;
}

void mysql_callback(WFMySQLTask *mysql_task)
{
    wfrest::Json json = mysql_concat_json_res(mysql_task);
    auto *server_resp = static_cast<HttpResp *>(mysql_task->user_data);
    server_resp->String(json.dump());
}

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

        const std::string &header = this->header("Content-Encoding");
        int status = StatusOK;
        if (header.find("gzip") != std::string::npos)
        {
            status = Compressor::ungzip(&content, &req_data_->body);
        }
        else
        {
            status = StatusNoUncomrpess;
        }
        if(status != StatusOK)
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

wfrest::Json &HttpReq::json() const
{
    if (content_type_ == APPLICATION_JSON && req_data_->json.empty())
    {
        const std::string &body_content = this->body();
        Json tmp = Json::parse(body_content);
        if (tmp.is_valid())
        {
            req_data_->json = std::move(tmp);
        }
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
        cookies_ = HttpCookie::split(cookie_piece);
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
    auto *compress_data = new std::string;
    int ret = this->compress(&str, compress_data);
    if(ret != StatusOK)
    {
        this->append_output_body(static_cast<const void *>(str.c_str()), str.size());
    } else
    {
        this->append_output_body_nocopy(compress_data->c_str(), compress_data->size());
    }
    task_of(this)->add_callback([compress_data](HttpTask *) { delete compress_data; });
}

void HttpResp::String(std::string &&str)
{
    auto *data = new std::string;
    int ret = this->compress(&str, data);
    if(ret != StatusOK)
    {
        *data = std::move(str);
    }
    this->append_output_body_nocopy(data->c_str(), data->size());
    task_of(this)->add_callback([data](HttpTask *) { delete data; });
}

void HttpResp::String(const MultiPartEncoder &multi_part_encoder)
{
    MultiPartEncoder *encoder = new MultiPartEncoder(multi_part_encoder);
    this->String(encoder);
}

void HttpResp::String(MultiPartEncoder &&multi_part_encoder)
{
    MultiPartEncoder *encoder = new MultiPartEncoder(std::move(multi_part_encoder));
    this->String(encoder);
}

void HttpResp::String(MultiPartEncoder *encoder)
{
    const std::string &boudary = encoder->boundary();
    this->headers["Content-Type"] = "multipart/form-data; boundary=" + boudary;

    HttpServerTask *server_task = task_of(this);
    SeriesWork *series = series_of(server_task);

    std::string *content = new std::string;
    series->set_context(content);
    series->set_callback([encoder](const SeriesWork *series)
    {
        delete encoder;
        delete static_cast<std::string *>(series->get_context());
    });

    const MultiPartEncoder::ParamList &param_list = encoder->params();
    int param_idx = 0;
    for(const auto &param : param_list)
    {
        if (param_idx != 0)
        {
            content->append("\r\n");
        }
        param_idx++;
        content->append("--");
        content->append(boudary);
        content->append("\r\nContent-Disposition: form-data; name=\"");
        content->append(param.first);
        content->append("\"\r\n\r\n");
        content->append(param.second);
    }
    const MultiPartEncoder::FileList &file_list = encoder->files();
    size_t file_cnt = file_list.size();
    assert(file_cnt >= 0);
    if (file_cnt == 0)
    {
        content->append("\r\n--");
        content->append(boudary);
        content->append("--\r\n");
        this->append_output_body_nocopy(content->c_str(), content->size());
    }
    size_t file_idx = 0;
    for(const auto &file : file_list)
    {
        if(!PathUtil::is_file(file.second))
        {
            fprintf(stderr, "[Error] Not a File : %s\n", file.second.c_str());
            continue;
        }
        size_t file_size;
        int ret = FileUtil::size(file.second, &file_size);
        if (ret != StatusOK)
        {
            fprintf(stderr, "[Error] Invalid File : %s\n", file.second.c_str());
            continue;
        }
        void *buf = malloc(file_size);
        server_task->add_callback([buf](const HttpTask *server_task)
                                {
                                    free(buf);
                                });
        WFFileIOTask *pread_task = WFTaskFactory::create_pread_task(file.second,
                buf, file_size, 0,
                [&file, &boudary, param_idx, file_idx](WFFileIOTask *pread_task)
                {
                    FileIOArgs *args = pread_task->get_args();
                    long ret = pread_task->get_retval();

                    SeriesWork *series = series_of(pread_task);
                    std::string *content = static_cast<std::string *>(series->get_context());
                    if (pread_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
                    {
                        fprintf(stderr, "Read %s Error\n", file.second.c_str());
                    } else
                    {
                        std::string file_suffix = PathUtil::suffix(file.second);
                        std::string file_type = ContentType::to_str_by_suffix(file_suffix);
                        if (param_idx != 0 || file_idx != 0)
                        {
                            content->append("\r\n");
                        }
                        content->append("--");
                        content->append(boudary);
                        content->append("\r\nContent-Disposition: form-data; name=\"");
                        content->append(file.first);
                        content->append("\"; filename=\"");
                        content->append(PathUtil::base(file.second));
                        content->append("\"\r\nContent-Type: ");
                        content->append(file_type);
                        content->append("\r\n\r\n");
                        content->append(static_cast<char *>(args->buf), ret);
                    }
                    // last one, send the content
                    if(pread_task->user_data) {
                        content->append("\r\n--");
                        content->append(boudary);
                        content->append("--\r\n");
                        HttpResp *resp = static_cast<HttpResp *>(pread_task->user_data);
                        resp->append_output_body_nocopy(content->c_str(), content->size());
                    }
                });
        if(file_idx == file_cnt - 1)
        {
            pread_task->user_data = this;
        }
        series->push_back(pread_task);
        file_idx++;
    }
}

int HttpResp::compress(const std::string * const data, std::string *compress_data)
{
    int status = StatusOK;
    if (headers.find("Content-Encoding") != headers.end())
    {
        if (headers["Content-Encoding"].find("gzip") != std::string::npos)
        {
            status = Compressor::gzip(data, compress_data);
        }
    } else
    {
        status = StatusNoComrpess;
    }
    return status;
}

void HttpResp::Error(int error_code)
{
    this->Error(error_code, "");
}

void HttpResp::Error(int error_code, const std::string &errmsg)
{
    int status_code = 503;
    switch (error_code)
    {
    case StatusNotFound:
    case StatusRouteVerbNotImplment:
    case StatusRouteNotFound:
        status_code = 404;
        break;
    default:
        break;
    }
    this->headers["Content-Type"] = "application/json";
    this->set_status(status_code);
    wfrest::Json js;
    std::string resp_msg = error_code_to_str(error_code);
    if(!errmsg.empty()) resp_msg = resp_msg + " : " + errmsg;
    if(CodeUtil::is_url_encode(errmsg))
    {
        resp_msg = CodeUtil::url_decode(resp_msg);
    }
    js["errmsg"] = resp_msg;

    this->Json(js);
}

void HttpResp::Timer(unsigned int microseconds, const TimerFunc &func)
{
    WFTimerTask *timer_task = WFTaskFactory::create_timer_task(microseconds,
                                                               [func](WFTimerTask *) { func(); });
    this->add_task(timer_task);
}

void HttpResp::Timer(time_t seconds, long nanoseconds, const TimerFunc &func)
{
    WFTimerTask *timer_task = WFTaskFactory::create_timer_task(seconds, nanoseconds,
                                                               [func](WFTimerTask *) { func(); });
    this->add_task(timer_task);
}

struct PushChunkData
{
    std::string data;
    size_t nleft = 0;
    HttpServerTask *server_task = nullptr;
};

void push_retry_callback(WFTimerTask *timer_task)
{
    auto* push_chunk_data = static_cast<PushChunkData *>(timer_task->user_data);
    auto* server_task = push_chunk_data->server_task;
    size_t nleft = push_chunk_data->nleft;
    size_t pos = push_chunk_data->data.size() - nleft;
    size_t nwritten = server_task->push(push_chunk_data->data.c_str() + pos, nleft);
    if (nwritten >= 0)
    {
        nleft = nleft - nwritten;
    } else {
        nwritten = 0;
        if (errno != EWOULDBLOCK)
        {
            delete push_chunk_data;
            return;
        }
    }
    if (nleft > 0)
    {
        push_chunk_data->nleft = nleft;
        timer_task = WFTaskFactory::create_timer_task(0, 1000000, push_retry_callback);
        timer_task->user_data = push_chunk_data;
        series_of(server_task)->push_front(timer_task);
    } else {
        // all the data has been sent
        delete push_chunk_data;
    }
}

struct PushTaskCtx
{
    HttpServerTask *server_task = nullptr;
    std::string cond_name;
    HttpResp::PushFunc push_cb;
    HttpResp::PushErrorFunc push_err_cb;
    std::string body()
    {
        std::string data;
        push_cb(data);
        // construct chunked data
        std::stringstream ss;
        if (!data.empty())
        {
            ss << std::hex << data.size() << "\r\n";
            ss << data << "\r\n";
        }
        else
        {
            ss << "0\r\n\r\n";
        }
        return ss.str();
    }
};

void push_func(WFCounterTask *push_task)
{
    auto *push_task_ctx = static_cast<PushTaskCtx *>(push_task->user_data);
    auto *server_task = push_task_ctx->server_task;
    auto *req = server_task->get_req();
    if (!req->is_keep_alive() || server_task->close_flag())
    {
        fprintf(stderr, "Close the connection\n");
        return;
    }
    // construct response
    std::string resp_body = push_task_ctx->body();
    size_t nleft = resp_body.size();
    size_t nwritten = server_task->push(resp_body.c_str(), resp_body.size());
    if (nwritten >= 0)
    {
        nleft = nleft - nwritten;
    } else {
        nwritten = 0;
        if (errno != EWOULDBLOCK)
        {
            push_task_ctx->push_err_cb();
            return;
        }
    }
    if (nleft > 0)
    {
        auto* push_chunk_data = new PushChunkData;
        push_chunk_data->data = std::move(resp_body);
        push_chunk_data->nleft = nleft;
        push_chunk_data->server_task = server_task;
        auto* timer_task = WFTaskFactory::create_timer_task(0, 1000000, push_retry_callback);
        timer_task->user_data = push_chunk_data;
        series_of(server_task)->push_front(timer_task);
    }
    push_task = WFTaskFactory::create_counter_task(0, push_func);
    push_task->user_data = push_task_ctx;
    auto *cond = WFTaskFactory::create_conditional(push_task_ctx->cond_name, push_task);
    **server_task << cond;
}

std::string HttpResp::construct_push_header()
{
    std::string http_header;
    http_header.reserve(128);
    http_header.append("HTTP/1.1 200 OK\r\n");
    if (headers.find("Transfer-Encoding") != headers.end())
    {
        headers.erase("Transfer-Encoding");
    }
    for (auto it = headers.begin(); it != headers.end(); it++)
    {
        const auto &key = it->first;
        const auto &val = it->second;
        http_header.append(key);
        http_header.append(": ");
        http_header.append(val);
        http_header.append("\r\n");
    }
    if (headers.find("Connection") == headers.end())
    {
        http_header.append("Connection: close\r\n");
    }
    http_header.append("Transfer-Encoding: chunked\r\n");
    http_header.append("\r\n");
    return http_header;
}

void HttpResp::Push(const std::string &cond_name, const PushFunc &push_cb)
{
    this->Push(cond_name, push_cb, [] {
        fprintf(stderr, "Connection has lost...\n");
    });
}

void HttpResp::Push(const std::string &cond_name, const PushFunc &push_cb, const PushErrorFunc &err_cb)
{
    HttpServerTask *server_task = task_of(this);
    // Construct HTTP header
    std::string http_header = construct_push_header();
    server_task->push(http_header.c_str(), http_header.size());

    auto* push_task_ctx = new PushTaskCtx;
    push_task_ctx->server_task = server_task;
    push_task_ctx->cond_name = cond_name;
    push_task_ctx->push_cb = push_cb;
    push_task_ctx->push_err_cb = err_cb;
    server_task->add_callback([push_task_ctx](HttpTask *server_task) {
        delete push_task_ctx;
    });
    auto* push_task = WFTaskFactory::create_counter_task(0, push_func);
    push_task->user_data = push_task_ctx;
    auto* cond = WFTaskFactory::create_conditional(cond_name, push_task);
    server_task->noreply();  // no need to send original response
    **server_task << cond;
}

void HttpResp::File(const std::string &path)
{
    this->File(path, 0, -1);
}

void HttpResp::File(const std::string &path, size_t start)
{
    this->File(path, start, -1);
}

void HttpResp::File(const std::string &path, size_t start, size_t end)
{
    int ret = HttpFile::send_file(path, start, end, this);
    if(ret != StatusOK)
    {
        this->Error(ret);
    }
}

void HttpResp::CachedFile(const std::string &path)
{
    this->CachedFile(path, 0, -1);
}

void HttpResp::CachedFile(const std::string &path, size_t start)
{
    this->CachedFile(path, start, -1);
}

void HttpResp::CachedFile(const std::string &path, size_t start, size_t end)
{
    int ret = HttpFile::send_cached_file(path, start, end, this);
    if(ret != StatusOK)
    {
        this->Error(ret);
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

void HttpResp::Save(const std::string &file_dst, const std::string &content, const std::string &notify_msg)
{
    HttpFile::save_file(file_dst, content, this, notify_msg);
}

void HttpResp::Save(const std::string &file_dst, std::string &&content, const std::string &notify_msg)
{
    HttpFile::save_file(file_dst, std::move(content), this, notify_msg);
}

void HttpResp::Save(const std::string &file_dst, const std::string &content,
        const HttpFile::FileIOArgsFunc &func)
{
    HttpFile::save_file(file_dst, content, this, func);
}

void HttpResp::Save(const std::string &file_dst, std::string &&content,
        const HttpFile::FileIOArgsFunc &func)
{
    HttpFile::save_file(file_dst, std::move(content), this, func);
}

void HttpResp::Json(const wfrest::Json &json)
{
    this->headers["Content-Type"] = "application/json";
    this->String(json.dump());
}

void HttpResp::Json(const std::string &str)
{
    if (!wfrest::Json::parse(str).is_valid())
    {
        this->Error(StatusJsonInvalid);
        return;
    }
    this->headers["Content-Type"] = "application/json";
    this->String(str);
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

void HttpResp::Http(const std::string &url, int redirect_max, size_t size_limit)
{
    HttpServerTask *server_task = task_of(this);
    HttpReq *server_req = server_task->get_req();
    std::string http_url = url;
	if (strncasecmp(url.c_str(), "http://", 7) != 0 &&
		strncasecmp(url.c_str(), "https://", 8) != 0)
	{
		http_url = "http://" + http_url;
	}
    WFHttpTask *http_task = WFTaskFactory::create_http_task(http_url,
                                                            redirect_max,
                                                            0,
                                                            proxy_http_callback);
    auto *proxy_ctx = new ProxyCtx;
    proxy_ctx->url = http_url;
    proxy_ctx->server_task = server_task;
    proxy_ctx->is_keep_alive = server_req->is_keep_alive();
    http_task->user_data = proxy_ctx;

    const void *body;
	size_t len;

    ParsedURI uri;
    // fprintf(stderr, "http_url : %s\n", http_url.c_str());
    if (URIParser::parse(http_url, uri) < 0)
    {
        server_task->get_resp()->set_status(HttpStatusBadRequest);
        return;
    }

    std::string route;
    if (uri.path && uri.path[0])
    {
        route.append(uri.path);
    } else
    {
        route.append("/");
    }

    if(uri.query && uri.query[0])
    {
        route.append("?");
        route.append(uri.query);
    }

	server_req->set_request_uri(route);
	server_req->get_parsed_body(&body, &len);
	server_req->append_output_body_nocopy(body, len);
    // Keep parts unique to HttpReq
    HttpRequest *server_req_cast = static_cast<HttpRequest *>(server_req);
	*http_task->get_req() = std::move(*server_req_cast);
    http_task->get_resp()->set_size_limit(size_limit);
	**server_task << http_task;
}

void HttpResp::MySQL(const std::string &url, const std::string &sql)
{
    WFMySQLTask *mysql_task = WFTaskFactory::create_mysql_task(url, 0, mysql_callback);
    mysql_task->get_req()->set_query(sql);
    mysql_task->user_data = this;
    this->add_task(mysql_task);
}

void HttpResp::MySQL(const std::string &url, const std::string &sql, const MySQLJsonFunc &func)
{
    WFMySQLTask *mysql_task = WFTaskFactory::create_mysql_task(url, 0,
    [func](WFMySQLTask *mysql_task)
    {
        wfrest::Json json = mysql_concat_json_res(mysql_task);
        func(&json);
    });

    mysql_task->get_req()->set_query(sql);
    this->add_task(mysql_task);
}

void HttpResp::MySQL(const std::string &url, const std::string &sql, const MySQLFunc &func)
{
    WFMySQLTask *mysql_task = WFTaskFactory::create_mysql_task(url, 0,
    [func](WFMySQLTask *mysql_task)
    {
        if (mysql_task->get_state() != WFT_STATE_SUCCESS)
        {
            std::string errmsg = WFGlobal::get_error_string(mysql_task->get_state(),
                                                mysql_task->get_error());
            auto *server_resp = static_cast<HttpResp *>(mysql_task->user_data);
            server_resp->String(std::move(errmsg));
            return;
        }
        MySQLResponse *mysql_resp = mysql_task->get_resp();
        MySQLResultCursor cursor(mysql_resp);
        func(&cursor);
    });
    mysql_task->get_req()->set_query(sql);
    mysql_task->user_data = this;
    this->add_task(mysql_task);
}

void HttpResp::Redis(const std::string &url, const std::string &command,
        const std::vector<std::string>& params)
{
    WFRedisTask *redis_task = WFTaskFactory::create_redis_task(url, 2, [this](WFRedisTask *redis_task)
    {
        wfrest::Json js = redis_json_res(redis_task);
        this->Json(js);
    });
	redis_task->get_req()->set_request(command, params);
    this->add_task(redis_task);
}

void HttpResp::Redis(const std::string &url, const std::string &command,
        const std::vector<std::string>& params, const RedisJsonFunc &func)
{
    WFRedisTask *redis_task = WFTaskFactory::create_redis_task(url, 2, [func](WFRedisTask *redis_task)
    {
        wfrest::Json js = redis_json_res(redis_task);
        func(&js);
    });
	redis_task->get_req()->set_request(command, params);
    this->add_task(redis_task);
}

void HttpResp::Redis(const std::string &url, const std::string &command,
        const std::vector<std::string>& params, const RedisFunc &func)
{
    WFRedisTask *redis_task = WFTaskFactory::create_redis_task(url, 2, func);
	redis_task->get_req()->set_request(command, params);
    this->add_task(redis_task);
}

void HttpResp::Redirect(const std::string& location, int status_code)
{
    this->headers["Location"] = location;
    this->set_status(status_code);
}

void HttpResp::add_task(SubTask *task)
{
    HttpServerTask *server_task = task_of(this);
    **server_task << task;
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

} // namespace wfrest

