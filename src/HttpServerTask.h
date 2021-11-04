#ifndef _HTTPSERVERTASK_H_
#define _HTTPSERVERTASK_H_

#include "HttpMsg.h"
#include "workflow/HttpUtil.h"
#include "workflow/HttpMessage.h"

namespace wfrest
{

    using WebTask = WFNetworkTask<HttpReq, HttpResp>;

    class HttpServerTask : public WFServerTask<HttpReq, HttpResp>
    {
    public:
        HttpServerTask(CommService *service,
                       std::function<void(WebTask *)> &process);

    protected:
        void handle(int state, int error) override;

        CommMessageOut *message_out() override;

    private:
        bool req_is_alive_;
        bool req_has_keep_alive_header_;
        std::string req_keep_alive_;
    };

} // namespace wfrest


#endif // _HTTPSERVERTASK_H_