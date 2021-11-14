#ifndef _HTTPSERVERTASK_H_
#define _HTTPSERVERTASK_H_

#include "HttpMsg.h"
#include "ServerTask.h"
#include "NetworkTask.h"
#include <workflow/HttpUtil.h>
#include <workflow/HttpMessage.h>

namespace wfrest
{
    using HttpTask = NetworkTask<HttpReq, HttpResp>;

    class HttpServerTask : public ServerTask<HttpReq, HttpResp>
    {
    public:
        using ProcCallBack = std::function<void(HttpTask *)>;
        HttpServerTask(CommService *service, ProcCallBack &process);

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