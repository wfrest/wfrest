//
// Created by Chanchan on 11/13/21.
//

#ifndef _SERVERBASE_H_
#define _SERVERBASE_H_

#include <workflow/WFServer.h>
#include "NetworkTask.h"
#include "ServerTask.h"

namespace wfrest
{

    template<class REQ, class RESP>
    class ServerBase : public WFServerBase
    {
    public:
        using ProcFunc = std::function<void(NetworkTask<REQ, RESP> *)>;

        ServerBase(const struct WFServerParams *params,
                   ProcFunc proc) :
                WFServerBase(params),
                process_(std::move(proc))
        {
        }

        explicit ServerBase(ProcFunc proc) :
                WFServerBase(&SERVER_PARAMS_DEFAULT),
                process_(std::move(proc))
        {
        }

    protected:
        virtual CommSession *new_session(long long seq, CommConnection *conn);

    protected:
        ProcFunc process_;
    };

    template<class REQ, class RESP>
    CommSession *ServerBase<REQ, RESP>::new_session(long long int seq, CommConnection *conn)
    {

        auto *task = new ServerTask<REQ, RESP>(this,
                                               WFGlobal::get_scheduler(),
                                               process_);;
        task->set_keep_alive(this->params.keep_alive_timeout);
        task->set_receive_timeout(this->params.receive_timeout);
        task->get_req()->set_size_limit(this->params.request_size_limit);

        return task;
    }

}  // namespace wfrest


#endif //_SERVERBASE_H_
