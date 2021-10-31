#ifndef _WFWEBSERVERTASK_H_
#define _WFWEBSERVERTASK_H_

#include "WFHttpMsg.h"
#include "workflow/HttpUtil.h"
#include "workflow/HttpMessage.h"

namespace wfrest
{

    class WFWebServerTask : public WFServerTask<HttpReq, HttpResp>
    {
    public:
        WFWebServerTask(CommService *service,
                        std::function<void(WFWebTask *)> &process);

    protected:
        void handle(int state, int error) override;

        CommMessageOut *message_out() override;

    private:
        bool req_is_alive_;
        bool req_has_keep_alive_header_;
        std::string req_keep_alive_;
    };

    class WFWebTaskFactory
    {
    public:
        static WFWebTask *create_web_task(CommService *service,
                                          std::function<void(WFWebTask *)> &process)
        {
            return new WFWebServerTask(service, process);
        }
    };


} // namespace wfrest


#endif // _WFWEBSERVERTASK_H_