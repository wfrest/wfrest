#ifndef WFREST_HTTPSERVERTASK_H_
#define WFREST_HTTPSERVERTASK_H_

#include "wfrest/HttpMsg.h"

namespace wfrest
{

class HttpServerTask : public WFServerTask<HttpReq, HttpResp>
{
public:
    using ProcFunc = std::function<void(HttpTask *)>;
    using ServerCallBack = std::function<void(HttpTask *)>;

    using WFServerTask::Series;
    // todo : if we remove & here, leads to coredump
    HttpServerTask(CommService *service, ProcFunc& process);

    void add_callback(const ServerCallBack &cb)
    { cb_list_.push_back(cb); }

    void add_callback(ServerCallBack &&cb)
    { cb_list_.emplace_back(std::move(cb)); }

    static size_t get_resp_offset()
    {
        HttpServerTask task(nullptr);
        return task.resp_offset();
    }

protected:
    void handle(int state, int error) override;

    CommMessageOut *message_out() override;

private:
    // for hidning set_callback
    void set_callback() {}

    size_t resp_offset() const
    {
        return (const char *) (&this->resp) - (const char *) this;
    }
    // Just be convinient for get_resp_offset
    HttpServerTask(std::function<void (HttpTask *)> proc):
        WFServerTask(nullptr, nullptr, proc)
    {}
private:
    bool req_is_alive_;
    bool req_has_keep_alive_header_;
    std::string req_keep_alive_;
    std::vector<ServerCallBack> cb_list_;
};

template<typename T>
inline HttpServerTask *task_of(T *task)
{
    auto *series = static_cast<HttpServerTask::Series *>(series_of(task));
    return static_cast<HttpServerTask *>(series->task);
}

template<>
inline HttpServerTask *task_of(HttpResp *resp)
{
    size_t http_resp_offset = HttpServerTask::get_resp_offset();
    return (HttpServerTask *) ((char *) (resp) - http_resp_offset);
}

} // namespace wfrest


#endif // WFREST_HTTPSERVERTASK_H_