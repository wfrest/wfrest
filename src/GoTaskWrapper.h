#ifndef WFREST_GoTaskWrapper_H_
#define WFREST_GoTaskWrapper_H_

#include "workflow/WFFacilities.h"
#include "HttpServerTask.h"

namespace wfrest
{

#define go GoTaskWrapper()-

struct GoTaskWrapper
{
    template <typename Function>
    inline void operator-(Function func)
    {
        auto* go_task = new __WFGoTask(WFGlobal::get_exec_queue("go"),
                              WFGlobal::get_compute_executor(),
                              std::move(func));
        auto *server_task = HttpServerTask::get_thread_local_task();
        **server_task << go_task;
    }
};

}  // namespace wfrest

#endif // WFREST_GoTaskWrapper_H_
