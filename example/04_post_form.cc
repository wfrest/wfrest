#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"

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

    // Urlencoded Form
    // curl -v http://ip:port/post -H "content-type:application/x-www-form-urlencoded" -d 'user=admin&pswd=123456'
    svr.Post("/post", [](const HttpReq *req, HttpResp *resp)
    {
        if(req->content_type != APPLICATION_URLENCODED)
        {
            resp->set_status(HttpStatusBadRequest);
            return;
        }
        auto& form_kv = req->kv;
        for(auto& kv : form_kv)
        {
            fprintf(stderr, "key %s : vak %s\n", kv.first.c_str(), kv.second.c_str());
        }
    });

    // curl -X POST http://ip:port/form \
    // -F "file=@/path/file" \
    // -H "Content-Type: multipart/form-data"
    svr.Post("/form", [](const HttpReq *req, HttpResp *resp)
    {
        if(req->content_type != MULTIPART_FORM_DATA)
        {
            resp->set_status(HttpStatusBadRequest);
            return;
        }
        // form_kv's type is MultiPart
        // MultiPart = std::unordered_map<std::string, FormData>;
        /*
            struct FormData
            {
                std::string filename;
                std::string content;
            };
        */
        auto& form_kv = req->form;
        for(auto & it : form_kv)
        {
            fprintf(stderr, "%s : %s = %s",
                                it.first.c_str(),
                                it.second.content.c_str(),
                                it.second.filename.c_str());
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
