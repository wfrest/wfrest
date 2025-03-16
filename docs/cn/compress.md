## 压缩

```cpp
// 服务器
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;
    
    // 1. 您可以使用 `./13_compess_client` 
    // 2. 或者使用Python脚本 `python3 13_compress_client.py`
    // 3. echo '{"testgzip": "gzip compress data"}' | gzip |  \
    // curl -v -i --data-binary @- -H "Content-Encoding: gzip" http://ip:port/gzip
    svr.POST("/gzip", [](const HttpReq *req, HttpResp *resp)
    {
        // 我们自动解压从客户端发送的压缩数据
        // 目前仅支持gzip和br格式
        std::string& data = req->body();
        fprintf(stderr, "ungzip data : %s\n", data.c_str());
        resp->set_compress(Compress::GZIP);
        resp->String("测试服务器发送gzip数据\n");
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
// 客户端
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
    fprintf(stderr, "解压数据 : %s", decompress_data.c_str());
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
    std::string content = "客户端发送测试Gzip";
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

关于如何编写客户端，您可以参考[workflow](https://github.com/sogou/workflow)，它目前支持HTTP、Redis、MySQL和Kafka协议。您可以使用它来编写高效的异步客户端。 