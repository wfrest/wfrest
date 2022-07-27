#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"
#include <signal.h>
#include "wfrest/Compress.h"
#include "wfrest/ErrorCode.h"

using namespace protocol;
using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main()
{
    signal(SIGINT, sig_handler);

    WFHttpTask *task = WFTaskFactory::create_http_task("http://127.0.0.1:8888/你好",
                                                       4,
                                                       2,
                                                       nullptr);
    task->start();
    wait_group.wait();
}
