#include "workflow/HttpUtil.h"
#include "workflow/HttpMessage.h"

#include <arpa/inet.h>
#include <cstdio>
#include <cstring>

#include "HttpServerTask.h"
#include "HttpServer.h"
#include "HttpHeaderUtil.h"
#include "HttpKeepAliveUtil.h"

using namespace protocol;


namespace wfrest
{

namespace
{

bool is_supported_http_version(const char *version)
{
    return version != nullptr &&
           (std::strcmp(version, "HTTP/1.0") == 0 ||
            std::strcmp(version, "HTTP/1.1") == 0);
}

bool parse_status_code(const char *text, int *status_code)
{
    if (text == nullptr)
        return false;

    int value = 0;
    for (size_t i = 0; i < 3; ++i)
    {
        const unsigned char ch = static_cast<unsigned char>(text[i]);
        if (ch < '0' || ch > '9')
            return false;
        value = value * 10 + (ch - '0');
    }

    if (text[3] != '\0' || value < 100)
        return false;

    *status_code = value;
    return true;
}

bool is_valid_reason_phrase(const char *phrase)
{
    if (phrase == nullptr)
        return false;

    const auto *cursor = reinterpret_cast<const unsigned char *>(phrase);
    while (*cursor != '\0')
    {
        const unsigned char ch = *cursor++;
        const bool valid = ch == '\t' || ch == ' ' ||
                           (ch >= 0x21 && ch <= 0x7e) || ch >= 0x80;
        if (!valid)
            return false;
    }
    return true;
}

void sanitize_response_start_line(HttpResp *resp)
{
    if (!is_supported_http_version(resp->get_http_version()))
        resp->set_http_version("HTTP/1.1");

    const char *status_text = resp->get_status_code();
    if (status_text == nullptr)
    {
        HttpUtil::set_response_status(resp, HttpStatusOK);
        return;
    }

    int status_code;
    if (!parse_status_code(status_text, &status_code))
    {
        HttpUtil::set_response_status(resp, HttpStatusInternalServerError);
        return;
    }

    if (!is_valid_reason_phrase(resp->get_reason_phrase()))
        HttpUtil::set_response_status(resp, status_code);
}

} // namespace

HttpServerTask::HttpServerTask(CommService *service,
                               ProcFunc& process) :
        WFServerTask(service, WFGlobal::get_scheduler(), process),
        req_is_alive_(false),
        req_has_keep_alive_header_(false)
{
    WFServerTask::set_callback([this](HttpTask *task) {
        for(auto &cb : cb_list_)
        {
            cb(task);
        }
    });
}

void HttpServerTask::handle(int state, int error)
{
    if (state == WFT_STATE_TOREPLY)
    {
        req_is_alive_ = this->req.is_keep_alive();
        if (req_is_alive_ && this->req.has_keep_alive_header())
        {
            HttpHeaderCursor req_cursor(&this->req);
            struct HttpMessageHeader header{};

            header.name = "Keep-Alive";
            header.name_len = strlen("Keep-Alive");
            req_has_keep_alive_header_ = req_cursor.find(&header);
            if (req_has_keep_alive_header_)
            {
                req_keep_alive_.assign((const char *) header.value,
                                       header.value_len);
            }
        }
    }
    this->WFServerTask::handle(state, error);
}


CommMessageOut *HttpServerTask::message_out()
{
    HttpResp *resp = this->get_resp();
    sanitize_response_start_line(resp);

    std::map<std::string, std::string, MapStringCaseLess> &headers = resp->headers;
    detail::sanitize_application_response_headers(&headers);
    // content type
    if(headers.find("Content-Type") == headers.end())
    {
        headers["Content-Type"] = "text/plain";
    }
    if(headers.find("Date") == headers.end())
    {
        headers["Date"] = Timestamp::now().to_utc_format_str(
            "%a, %d %b %Y %H:%M:%S GMT");
    }
    struct HttpMessageHeader header;

    // fill headers we set
    for(auto &header_kv : headers)
    {
        header.name = header_kv.first.c_str();
        header.name_len = header_kv.first.size();
        header.value = header_kv.second.c_str();
        header.value_len = header_kv.second.size();
        resp->protocol::HttpResponse::add_header(&header);
    }
    // fill cookie
    for(auto &cookie : resp->cookies())
    {
        std::string cookie_str = cookie.dump();
        if (cookie_str.empty())
            continue;

        header.name = "Set-Cookie";
        header.name_len = 10;
        header.value = cookie_str.c_str();
        header.value_len = cookie_str.size();
        resp->protocol::HttpResponse::add_header(&header);
    }

    if (!resp->is_chunked() && !resp->has_content_length_header())
    {
        char buf[32];
        header.name = "Content-Length";
        header.name_len = strlen("Content-Length");
        header.value = buf;
        header.value_len = sprintf(buf, "%zu", resp->get_output_body_size());
        resp->protocol::HttpResponse::add_header(&header);
    }

    bool is_alive;

    if (resp->has_connection_header())
        is_alive = resp->is_keep_alive();
    else
        is_alive = req_is_alive_;

    if (!is_alive)
        this->keep_alive_timeo = 0;
    else
    {
        static const std::string empty_keep_alive;
        const std::string &keep_alive = req_has_keep_alive_header_
                                            ? req_keep_alive_
                                            : empty_keep_alive;
        this->keep_alive_timeo = detail::resolve_keep_alive_timeout(
            keep_alive, this->get_seq(), this->keep_alive_timeo);
    }

    if (!resp->has_connection_header())
    {
        header.name = "Connection";
        header.name_len = 10;
        if (this->keep_alive_timeo == 0)
        {
            header.value = "close";
            header.value_len = 5;
        } else
        {
            header.value = "Keep-Alive";
            header.value_len = 10;
        }

        resp->protocol::HttpResponse::add_header(&header);
    }
    return this->WFServerTask::message_out();
}

std::string HttpServerTask::peer_addr() const
{
    struct sockaddr_storage addr{};
    socklen_t addr_len = sizeof addr;
    if (this->get_peer_addr(reinterpret_cast<struct sockaddr *>(&addr),
                            &addr_len) != 0)
        return "Unknown";

    const void *binary_addr = nullptr;
    int family = addr.ss_family;
    if (family == AF_INET && addr_len >= sizeof(struct sockaddr_in))
    {
        const auto *sin = reinterpret_cast<const struct sockaddr_in *>(&addr);
        binary_addr = &sin->sin_addr;
    } else if (family == AF_INET6 &&
               addr_len >= sizeof(struct sockaddr_in6))
    {
        const auto *sin6 =
            reinterpret_cast<const struct sockaddr_in6 *>(&addr);
        binary_addr = &sin6->sin6_addr;
    } else
        return "Unknown";

    char addrstr[INET6_ADDRSTRLEN];
    if (inet_ntop(family, binary_addr, addrstr, sizeof addrstr) == nullptr)
        return "Unknown";
    return addrstr;
}

unsigned short HttpServerTask::peer_port() const
{
    struct sockaddr_storage addr{};
    socklen_t addr_len = sizeof addr;
    if (this->get_peer_addr(reinterpret_cast<struct sockaddr *>(&addr),
                            &addr_len) != 0)
        return 0;

    if (addr.ss_family == AF_INET &&
        addr_len >= sizeof(struct sockaddr_in))
    {
        const auto *sin = reinterpret_cast<const struct sockaddr_in *>(&addr);
        return ntohs(sin->sin_port);
    } else if (addr.ss_family == AF_INET6)
    {
        if (addr_len >= sizeof(struct sockaddr_in6))
        {
            const auto *sin6 =
                reinterpret_cast<const struct sockaddr_in6 *>(&addr);
            return ntohs(sin6->sin6_port);
        }
    }
    return 0;
}


bool HttpServerTask::close_flag() const
{
    return server->close_flag_;
}

} // namespace wfrest
