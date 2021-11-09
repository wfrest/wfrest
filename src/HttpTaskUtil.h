//
// Created by Chanchan on 11/3/21.
//

#ifndef _HTTPTASKUTIL_H_
#define _HTTPTASKUTIL_H_

#include "HttpMsg.h"

namespace wfrest
{

    namespace detail
    {
        class WebTaskModel : public WFNetworkTask<HttpReq, HttpResp>
        {
        public:
            static size_t get_req_offset()
            {
                WebTaskModel task(nullptr);

                return task.req_offset();
            }

            static size_t get_resp_offset()
            {
                WebTaskModel task(nullptr);

                return task.resp_offset();
            }

        protected:
            // Just for get rid of abstract
            WFConnection *get_connection() const override
            { return nullptr; }

            CommMessageIn *message_in() override
            { return &this->req; };

            CommMessageOut *message_out() override
            { return &this->resp; };

        private:
            explicit WebTaskModel(std::function<void(WebTask * )> proc) :
                    WFNetworkTask(nullptr, nullptr, std::move(proc))
            {}

            size_t req_offset() const
            {
                return (const char *) (&this->req) - (const char *) this;
            }

            size_t resp_offset() const
            {
                return (const char *) (&this->resp) - (const char *) this;
            }
        };

    }    // detauil

    static inline WebTask *task_of(const HttpReq *req)
    {
        size_t http_req_offset = detail::WebTaskModel::get_req_offset();
        return (WebTask *) ((char *) (req) - http_req_offset);
    }

    static inline WebTask *task_of(const HttpResp *resp)
    {
        size_t http_resp_offset = detail::WebTaskModel::get_resp_offset();
        return (WebTask *) ((char *) (resp) - http_resp_offset);
    }

}   // wftest

#endif //_HTTPTASKUTIL_H_
