## Upload Files 

```cpp
#include "wfrest/HttpServer.h"
#include "wfrest/PathUtil.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // Upload a file to parent dir is really dangerous.:
    // curl -v -X POST "ip:port/upload" -F "file=@demo.txt; filename=../demo.txt" -H "Content-Type: multipart/form-data"
    // Then you find the file is store in the parent dir, which is dangerous
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
                // filename : filecontent
                std::pair<std::string, std::string>& fileinfo = part.second;
                // file->filename SHOULD NOT be trusted. See Content-Disposition on MDN
                // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition#directives
                // The filename is always optional and must not be used blindly by the application:
                // path information should be stripped, and conversion to the server file system rules should be done.
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
