## Server Push data

To push data from server to client actively (using server sent event as an example)

You need to fill in the required headers, call the Push interface, and fill in the content you want to send in the callback. 

We will use chunked encoding to send the data by default.

You can see example/27_sse.cc for more detail.

Here's the simple usage:

```cpp
svr.GET("/sse", [](const HttpReq *req, HttpResp *resp)
{
    // sse header required
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
        body.append("price);
        body.append("\n\n");
    });
});
```
