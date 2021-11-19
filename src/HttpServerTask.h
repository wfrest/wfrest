#ifndef _HTTPSERVERTASK_H_
#define _HTTPSERVERTASK_H_

#include "workflow/HttpUtil.h"
#include "workflow/HttpMessage.h"

#include "HttpMsg.h"

namespace wfrest
{
class HttpServerTask : public WFServerTask<HttpReq, HttpResp>
{
public:
    using ProcFunc = std::function<void(HttpTask *)>;
    using ServerCallBack = std::function<void(HttpTask *)>;

    HttpServerTask(CommService *service, ProcFunc &process);

    void add_callback(const ServerCallBack &cb)
    { cb_list_.push_back(cb); }

    void add_callback(ServerCallBack &&cb)
    { cb_list_.emplace_back(std::move(cb)); }

protected:
    void handle(int state, int error) override;

    CommMessageOut *message_out() override;

private:
    // for hidning set_callback
    void set_callback()
    {}

private:
    bool req_is_alive_;
    bool req_has_keep_alive_header_;
    std::string req_keep_alive_;
    std::vector<ServerCallBack> cb_list_;
};

} // namespace wfrest


#endif // _HTTPSERVERTASK_H_