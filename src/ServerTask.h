//
// Created by Chanchan on 11/13/21.
//

#ifndef _SERVERTASK_H_
#define _SERVERTASK_H_

#include "NetworkTask.h"

namespace wfrest
{
    template<class REQ, class RESP>
    class ServerTask : public NetworkTask<REQ, RESP>
    {
    public:
        using ProcCallBack = std::function<void(NetworkTask<REQ, RESP> *)>;

        ServerTask(CommService *service, CommScheduler *scheduler, ProcCallBack &proc);

    protected:
        virtual ~ServerTask() = default;

        virtual CommMessageOut *message_out()
        { return &this->resp; }

        virtual CommMessageIn *message_in()
        { return &this->req; }

        virtual void handle(int state, int error);

        virtual void dispatch();

        virtual WFConnection *get_connection() const;

    protected:
        class Processor : public SubTask
        {
        public:
            Processor(ServerTask<REQ, RESP> *task, ProcCallBack &proc);

            virtual void dispatch();

            virtual SubTask *done();

        public:
            ServerTask<REQ, RESP> *task_;
            ProcCallBack &process_;
        };

        class Series : public SeriesWork
        {
        public:
            explicit Series(ServerTask<REQ, RESP> *task);
            virtual ~Series();
        public:
            CommService *service_;
        };

    protected:
        Processor processor_;
        CommService *service_;
    };

    template<class REQ, class RESP>
    ServerTask<REQ, RESP>::Series::Series(ServerTask<REQ, RESP> *task)
            :SeriesWork(&task->processor_, nullptr)
    {
        this->set_last_task(task);
        service_= task->service_;
        service_->incref();
    }

    template<class REQ, class RESP>
    ServerTask<REQ, RESP>::Series::~Series()
    {
        this->callback = nullptr;
        service_->decref();
    }

    template<class REQ, class RESP>
    ServerTask<REQ, RESP>::Processor::Processor(ServerTask<REQ, RESP> *task, ServerTask::ProcCallBack &proc)
            :  task_(task), process_(proc)
    {}

    template<class REQ, class RESP>
    void ServerTask<REQ, RESP>::Processor::dispatch()
    {
        process_(task_);
        task_ = nullptr;    /* As a flag. get_conneciton() disabled. */
        this->subtask_done();
    }

    template<class REQ, class RESP>
    SubTask *ServerTask<REQ, RESP>::Processor::done()
    {
        return series_of(this)->pop();
    }


    template<class REQ, class RESP>
    ServerTask<REQ, RESP>::ServerTask(CommService *service, CommScheduler *scheduler, ServerTask::ProcCallBack &proc)
            : NetworkTask<REQ, RESP>(nullptr, scheduler),
              processor_(this, proc),
              service_(service)
    {}

    template<class REQ, class RESP>
    void ServerTask<REQ, RESP>::handle(int state, int error)
    {
        if (state == WFT_STATE_TOREPLY)
        {
            this->state = WFT_STATE_TOREPLY;
            this->target = this->get_target();
            new Series(this);
            processor_.dispatch();
        }
        else if (this->state == WFT_STATE_TOREPLY)
        {
            this->state = state;
            this->error = error;
            if (error == ETIMEDOUT)
                this->timeout_reason = TOR_TRANSMIT_TIMEOUT;

            this->subtask_done();
        }
        else
            delete this;
    }

    template<class REQ, class RESP>
    void ServerTask<REQ, RESP>::dispatch()
    {
        if (this->state == WFT_STATE_TOREPLY)
        {
            /* Enable get_connection() again if the reply() call is success. */
            processor_.task_ = this;
            if (this->scheduler->reply(this) >= 0)
                return;

            this->state = WFT_STATE_SYS_ERROR;
            this->error = errno;
            processor_.task_ = nullptr;
        }

        this->subtask_done();
    }

    template<class REQ, class RESP>
    WFConnection *ServerTask<REQ, RESP>::get_connection() const
    {
        if (processor_.task_)
            return static_cast<WFConnection *>(this->CommSession::get_connection());

        errno = EPERM;
        return nullptr;
    }

}  // namespace wfrest

#endif //_SERVERTASK_H_
