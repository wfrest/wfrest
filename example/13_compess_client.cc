#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"
#include <signal.h>
#include "wfrest/Compress.h"
#include "wfrest/ErrorCode.h"

using namespace protocol;
using namespace wfrest;

struct CompessContext
{
    std::string data;
};

void http_callback(WFHttpTask *task)
{
    const void *body;
    size_t body_len;
    task->get_resp()->get_parsed_body(&body, &body_len);
    std::string decompress_data;
    int ret = Compressor::ungzip(static_cast<const char *>(body), body_len, &decompress_data);
    fprintf(stderr, "Decompress Data : %s", decompress_data.c_str());
    delete static_cast<CompessContext *>(task->user_data);
}

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main()
{
    signal(SIGINT, sig_handler);

    std::string url = "http://127.0.0.1:8888";

    WFHttpTask *task = WFTaskFactory::create_http_task(url + "/gzip",
                                                       4,
                                                       2,
                                                       http_callback);
    std::string content = "Client send for test Gzip";
    auto *ctx = new CompessContext;
    int ret = Compressor::gzip(&content, &ctx->data);
    if(ret != StatusOK)
    {
        ctx->data = std::move(content);
    }
    task->user_data = ctx;
    task->get_req()->set_method("POST");
    task->get_req()->add_header_pair("Content-Encoding", "gzip");
    task->get_req()->append_output_body_nocopy(ctx->data.c_str(), ctx->data.size());
    task->start();
    wait_group.wait();
}
