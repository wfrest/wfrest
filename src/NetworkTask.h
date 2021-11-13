//
// Created by Chanchan on 11/13/21.
//

#ifndef _NETWORKTASK_H_
#define _NETWORKTASK_H_

#include <functional>
#include <vector>

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
        std::vector<NetWorkCallBack> cb_list_;

    };


}   // namespace wfrest

#endif //_NETWORKTASK_H_
