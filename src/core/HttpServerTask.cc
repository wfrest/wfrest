#include "workflow/HttpUtil.h"
#include "workflow/HttpMessage.h"

#include <arpa/inet.h>

#include "HttpServerTask.h"
#include "HttpServer.h"
#include "StrUtil.h"

using namespace protocol;

#define HTTP_KEEPALIVE_DEFAULT    (60 * 1000)
#define HTTP_KEEPALIVE_MAX        (300 * 1000)


namespace wfrest
{

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

    std::map<std::string, std::string, MapStringCaseLess> &headers = resp->headers;
    // content type
    if(headers.find("Content-Type") == headers.end())
    {
        headers["Content-Type"] = "text/plain";
    }
    if(headers.find("Date") == headers.end())
    {
        headers["Date"] = Timestamp::now().to_format_str("%a, %d %b %Y %H:%M:%S GMT");
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
        header.name = "Set-Cookie";
        header.name_len = 10;
        header.value = cookie_str.c_str();
        header.value_len = cookie_str.size();
        resp->protocol::HttpResponse::add_header(&header);
    }

    if (!resp->get_http_version())
        resp->set_http_version("HTTP/1.1");

    const char *status_code_str = resp->get_status_code();
    if (!status_code_str || !resp->get_reason_phrase())
    {
        int status_code;

        if (status_code_str)
            status_code = atoi(status_code_str);
        else
            status_code = HttpStatusOK;

        HttpUtil::set_response_status(resp, status_code);
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
        //req---Connection: Keep-Alive
        //req---Keep-Alive: timeout=5,max=100

        if (req_has_keep_alive_header_)
        {
            int flag = 0;
            std::vector<std::string> params = StrUtil::split(req_keep_alive_, ',');

            for (const auto &kv: params)
            {
                std::vector<std::string> arr = StrUtil::split(kv, '=');
                if (arr.size() < 2)
                    arr.emplace_back("0");

                std::string key = StrUtil::strip(arr[0]);
                std::string val = StrUtil::strip(arr[1]);
                if (!(flag & 1) && strcasecmp(key.c_str(), "timeout") == 0)
                {
                    flag |= 1;
                    // keep_alive_timeo = 5000ms when Keep-Alive: timeout=5
                    this->keep_alive_timeo = 1000 * atoi(val.c_str());
                    if (flag == 3)
                        break;
                } else if (!(flag & 2) && strcasecmp(key.c_str(), "max") == 0)
                {
                    flag |= 2;
                    if (this->get_seq() >= atoi(val.c_str()))
                    {
                        this->keep_alive_timeo = 0;
                        break;
                    }

                    if (flag == 3)
                        break;
                }
            }
        }

        if ((unsigned int) this->keep_alive_timeo > HTTP_KEEPALIVE_MAX)
            this->keep_alive_timeo = HTTP_KEEPALIVE_MAX;
        //if (this->keep_alive_timeo < 0 || this->keep_alive_timeo > HTTP_KEEPALIVE_MAX)

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
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof addr;
    this->get_peer_addr(reinterpret_cast<struct sockaddr *>(&addr), &addr_len);

    static const int ADDR_STR_LEN = 128;
    char addrstr[ADDR_STR_LEN];
    if (addr.ss_family == AF_INET)
    {
        auto *sin = reinterpret_cast<struct sockaddr_in *>(&addr);
        inet_ntop(AF_INET, &sin->sin_addr, addrstr, ADDR_STR_LEN);
    } else if (addr.ss_family == AF_INET6)
    {
        auto *sin6 = reinterpret_cast<struct sockaddr_in6 *>(&addr);
        inet_ntop(AF_INET6, &sin6->sin6_addr, addrstr, ADDR_STR_LEN);
    } else
        strcpy(addrstr, "Unknown");

    return addrstr;
}

unsigned short HttpServerTask::peer_port() const
{
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof addr;
    this->get_peer_addr(reinterpret_cast<struct sockaddr *>(&addr), &addr_len);

    unsigned short port = 0;
    if (addr.ss_family == AF_INET)
    {
        auto *sin = reinterpret_cast<struct sockaddr_in *>(&addr);
        port = ntohs(sin->sin_port);
    } else if (addr.ss_family == AF_INET6)
    {
        auto *sin6 = reinterpret_cast<struct sockaddr_in6 *>(&addr);
        port = ntohs(sin6->sin6_port);
    }
    return port;
}


bool HttpServerTask::close_flag() const
{
    return server->close_flag_;
}

} // namespace wfrest
