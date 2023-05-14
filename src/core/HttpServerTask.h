#ifndef WFREST_HTTPSERVERTASK_H_
#define WFREST_HTTPSERVERTASK_H_

#include "HttpMsg.h"
#include "Noncopyable.h"

namespace wfrest
{

class HttpServer;

class HttpServerTask : public WFServerTask<HttpReq, HttpResp> , public Noncopyable
{
public:
    using ProcFunc = std::function<void(HttpTask *)>;
    using ServerCallBack = std::function<void(HttpTask *)>;

    using WFServerTask::Series;
    friend class HttpServer;
    // if we remove & here, leads to coredump
    HttpServerTask(CommService *service, ProcFunc &process);

    void add_callback(const ServerCallBack &cb)
    { cb_list_.push_back(cb); }

    void add_callback(ServerCallBack &&cb)
    { cb_list_.emplace_back(std::move(cb)); }

    static size_t get_resp_offset()
    {
        HttpServerTask task(nullptr);
        return task.resp_offset();
    }

    std::string peer_addr() const;

    unsigned short peer_port() const;
    
    bool close_flag() const;
protected:
    void handle(int state, int error) override;

    CommMessageOut *message_out() override;

private:
    // for hidning set_callback
    void set_callback()
    {}

    size_t resp_offset() const
    {
        return (const char *) (&this->resp) - (const char *) this;
    }

    // Just be convinient for get_resp_offset
    HttpServerTask(std::function<void(HttpTask *)> proc) :
            WFServerTask(nullptr, nullptr, proc)
    {}
    
private:
    bool req_is_alive_;
    bool req_has_keep_alive_header_;
    std::string req_keep_alive_;
    std::vector<ServerCallBack> cb_list_;
    HttpServer* server = nullptr;
};

inline HttpServerTask *task_of(const SubTask *task)
{
    auto *series = static_cast<HttpServerTask::Series *>(series_of(task));
    return static_cast<HttpServerTask *>(series->task);
}

inline HttpServerTask *task_of(const HttpResp *resp)
{
    size_t http_resp_offset = HttpServerTask::get_resp_offset();
    return (HttpServerTask *) ((char *) (resp) - http_resp_offset);
}

} // namespace wfrest


#endif // WFREST_HTTPSERVERTASK_H_
