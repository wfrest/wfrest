## 上传文件

```cpp
#include "wfrest/HttpServer.h"
#include "wfrest/PathUtil.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // 将文件上传到父目录是非常危险的：
    // curl -v -X POST "ip:port/upload" -F "file=@demo.txt; filename=../demo.txt" -H "Content-Type: multipart/form-data"
    // 然后你会发现文件被存储在父目录中，这是危险的
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
                // 文件名 : 文件内容
                std::pair<std::string, std::string>& fileinfo = part.second;
                // file->filename 不应该被信任。参见MDN上的Content-Disposition
                // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition#directives
                // 文件名始终是可选的，不应该被应用程序盲目使用：
                // 应该去除路径信息，并转换为服务器文件系统规则。
                if(fileinfo.first.empty())
                {
                    continue;
                }
                fprintf(stderr, "文件名 : %s\n", fileinfo.first.c_str());

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