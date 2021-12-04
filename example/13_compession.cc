#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;
    
    svr.POST("/gzip", [](HttpReq *req, HttpResp *resp)
    {
        std::string ungzip_data = req->ungzip();
        fprintf(stderr, "ungzip data : %s\n", ungzip_data.c_str());
        resp->String("Test for server send gzip data\n", Compress::GZIP);
    });

    if (svr.start(8888) == 0)
    {
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
