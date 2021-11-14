//
// Created by Chanchan on 11/13/21.
//

#ifndef _NETWORKTASK_H_
#define _NETWORKTASK_H_

#include <functional>
#include <vector>
#include <workflow/WFTask.h>

namespace wfrest
{
    template<class REQ, class RESP>
    class NetworkTask : public WFNetworkTask<REQ, RESP>
    {
    public:
        using NetWorkCallBack = std::function<void (NetworkTask<REQ, RESP> *)>;

        void add_callback(NetWorkCallBack cb)
        {
            cb_list_.emplace_back(std::move(cb));
        }
    protected:
        NetworkTask(CommSchedObject *object, CommScheduler *scheduler);
        virtual SubTask *done();
    protected:
        std::vector<NetWorkCallBack> cb_list_;
    };

    template<class REQ, class RESP>
    NetworkTask<REQ, RESP>::NetworkTask(CommSchedObject *object, CommScheduler *scheduler):
            WFNetworkTask<REQ, RESP>(object, scheduler, nullptr)
    {}

    template<class REQ, class RESP>
    SubTask *NetworkTask<REQ, RESP>::done()
    {
        SeriesWork *series = series_of(this);

        if (this->state == WFT_STATE_SYS_ERROR && this->error < 0)
        {
            this->state = WFT_STATE_SSL_ERROR;
            this->error = -this->error;
        }

        if (this->callback)
            this->callback(this);

        for(auto& cb : cb_list_)
        {
            cb(this);
        }

        delete this;
        return series->pop();
    }



}   // namespace wfrest

#endif //_NETWORKTASK_H_
