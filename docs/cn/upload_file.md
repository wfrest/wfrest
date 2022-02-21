## 上传文件

```cpp
#include "wfrest/HttpServer.h"
#include "wfrest/PathUtil.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    svr.POST("/upload", [](const HttpReq *req, HttpResp *resp)
    {
        Form &form = req->form();

        if (form.empty())
        {
            resp->set_status(HttpStatusBadRequest);
        } else
        {
            for(auto& part : form)
            {
                const std::string& name = part.first;
                std::pair<std::string, std::string>& fileinfo = part.second;
                if(fileinfo.first.empty())
                {
                    continue;
                }
                fprintf(stderr, "filename : %s\n", fileinfo.first.c_str());

                resp->Save(PathUtil::base(fileinfo.first), std::move(fileinfo.second));
            }
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

上传文件到服务器中，我们一般通过*multipart/form-data*的方式编码，所以我们通过`Form &form = req->form();`获取到Form

而Form的结构为:

```cpp
// <name ,<filename, body>>
using Form = std::map<std::string, std::pair<std::string, std::string>>;
```

我们通过`Save`接口即可写文件到本地服务器中。

```cpp
resp->Save(PathUtil::base(fileinfo.first), std::move(fileinfo.second));
```

## 注意:

file->filename不可信任，详细可见 https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition#directives

通过 `curl -v -X POST "ip:port/upload" -F "file=@demo.txt; filename=../demo.txt" -H "Content-Type: multipart/form-data"`

你可以发现你的文件被存储到了上一级的文件夹中，这个操作十分危险。




