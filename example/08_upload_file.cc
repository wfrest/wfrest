#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"
#include "wfrest/PathUtil.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;

    // An expriment (Upload a file to parent dir is really dangerous.):
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

                resp->Save(fileinfo.first, std::move(fileinfo.second));
            }
        }
    });

    // Here is the right way:
    // curl -v -X POST "ip:port/upload_fix" -F "file=@demo.txt; filename=../demo.txt" -H "Content-Type: multipart/form-data"

    // And you can test for upload multiple files
    // curl -X POST http://ip:port/upload_fix \
    // -F "file1=@file1" \
    // -F "file2=@file2" \
    // -H "Content-Type: multipart/form-data"
    svr.POST("/upload_fix", [](const HttpReq *req, HttpResp *resp)
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
                // simple solution to fix the problem above
                // This will restrict the upload file to current directory.
                resp->Save(PathUtil::base(fileinfo.first), std::move(fileinfo.second));
            }
        }
    });

    if (svr.start(8888) == 0)
    {
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}


