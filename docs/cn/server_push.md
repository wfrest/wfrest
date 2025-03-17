## 服务器推送数据

要从服务器主动向客户端推送数据（以服务器发送事件为例）

您需要填写必要的头部，调用Push接口，并在回调中填写您想要发送的内容。

我们将默认使用分块编码发送数据。

您可以查看example/27_sse.cc获取更多详细信息。

以下是简单用法：

```cpp
svr.GET("/sse", [](const HttpReq *req, HttpResp *resp)
{
    // sse头部必需
    resp->add_header("Content-Type", "text/event-stream");
    resp->add_header("Cache-Control", "no-cache");
    resp->add_header("Connection", "keep-alive");
    resp->Push("test", [](std::string &body) {
        body.reserve(128);
        body.append(": ");
        body.append("comment");
        body.append("\n");
        body.append("id: ");
        body.append(std::to_string(1));
        body.append("\n");
        body.append("event: ");
        body.append("message");
        body.append("\n");
        body.append("data: ");
        body.append("price");
        body.append("\n\n");
    });
});
``` 