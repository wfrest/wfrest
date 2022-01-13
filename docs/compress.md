## Compression

```cpp
// Server
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;
    
    // 1. You can `./13_compess_client` 
    // 2. or use python script `python3 13_compress_client.py`
    // 3. echo '{"testgzip": "gzip compress data"}' | gzip |  \
    // curl -v -i --data-binary @- -H "Content-Encoding: gzip" http://ip:port/gzip
    svr.POST("/gzip", [](const HttpReq *req, HttpResp *resp)
    {
        // We automatically decompress the compressed data sent from the client
        // Support gzip, br only now
        std::string& data = req->body();
        fprintf(stderr, "ungzip data : %s\n", data.c_str());
        resp->set_compress(Compress::GZIP);
        resp->String("Test for server send gzip data\n");
    });

    if (svr.start(8888) == 0)
    {
        getchar();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
```

```cpp
// Client
#include "workflow/WFTaskFactory.h"
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
    getchar();
}
```

As for how to write a client, you can see [workflow](https://github.com/sogou/workflow) which currently supports HTTP, Redis, MySQL and Kafka protocols. You can use it to write efficient asynchronous clients.
