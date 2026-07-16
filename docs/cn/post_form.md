## 表单提交

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // URL编码表单
    // curl -v http://ip:port/post \
    // -H "body-type:application/x-www-form-urlencoded" \
    // -d 'user=admin&pswd=123456'
    svr.POST("/post", [](const HttpReq *req, HttpResp *resp)
    {
        if (req->content_type() != APPLICATION_URLENCODED)
        {
            resp->set_status(HttpStatusBadRequest);
            return;
        }
        std::map<std::string, std::string> &form_kv = req->form_kv();
        for (auto &kv: form_kv)
        {
            fprintf(stderr, "键 %s : 值 %s\n", kv.first.c_str(), kv.second.c_str());
        }
    });

    // curl -X POST http://ip:port/form \
    // -F "file=@/path/file" \
    // -H "Content-Type: multipart/form-data"
    svr.POST("/form", [](const HttpReq *req, HttpResp *resp)
    {
        if (req->content_type() != MULTIPART_FORM_DATA)
        {
            resp->set_status(HttpStatusBadRequest);
            return;
        }
        /*
            // https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST
            // <name ,<filename, body>>
            using Form = std::map<std::string, std::pair<std::string, std::string>>;
        */
        const Form &form = req->form();
        for (auto &it: form)
        {
            auto &name = it.first;
            auto &file_info = it.second;
            fprintf(stderr, "%s : %s = %s",
                    name.c_str(),
                    file_info.first.c_str(),
                    file_info.second.c_str());
        }
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

Content-Type 的媒体类型和 multipart 参数名均按不区分大小写的方式匹配。
multipart 请求必须且只能提供一个有效的 `boundary` 参数；带引号的边界、可选空白，
以及边界前后的其他参数都可以正常处理。边界缺失或无效、请求体未到达完整结束边界时，
`req->form()` 返回空表单，避免业务处理器读取畸形上传中的部分数据。

## 多部分表单编码器

URL 编码表单的字段名和值与查询参数使用相同规则：`+` 转换为空格；字段切分后
再解码有效的 `%xx`；值中的 `=` 会保留；错误的转义保持原样；解码后字段名
重复时保留第一个值。

使用MultiPartEncoder来编码多部分数据格式内容并发送。

```cpp
svr.GET("/form_send", [](const HttpReq *req, HttpResp *resp)
{
    MultiPartEncoder encoder;
    encoder.add_param("Filename", "1.jpg");
    encoder.add_file("test_1.txt", "./www/test_1.txt");
    encoder.add_file("test_2.txt", "./www/test_2.txt");
    resp->String(std::move(encoder));
});
```
