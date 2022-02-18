## 表单数据

表单数据比较常用的是 x-www-form-urlencoded 和 multipart/form-data

如果是`x-www-form-urlencoded`，调用`req->form_kv()` 获取kv数据，数据结构为`std::map<std::string, std::string>`

如果是`multipart/form-data`, req->form()

数据结构为

```cpp
// <name ,<filename, body>>
using Form = std::map<std::string, std::pair<std::string, std::string>>;
```

## 示例

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

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
            fprintf(stderr, "key %s : vak %s\n", kv.first.c_str(), kv.second.c_str());
        }
    });


    svr.POST("/form", [](const HttpReq *req, HttpResp *resp)
    {
        if (req->content_type() != MULTIPART_FORM_DATA)
        {
            resp->set_status(HttpStatusBadRequest);
            return;
        }
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
