# wfrest Web Framework

The c++ async micro framework for building web applications based on workflow

## Contents

- [wfrest Web Framework](#wfrest-web-framework)
    - [Contents](#contents)
    - [Build](#build)
    - [Quick start](#quick-start)
    - [API Examples](#api-examples)
      - [Parameters in path](#parameters-in-path)
      - [Querystring parameters](#querystring-parameters)
      - [Post Form](#post-form)
      - [Header](#header)
      - [Send File](#send-file)
      - [Save File](#save-file)
      - [Upload Files](#upload-files)
      - [Json](#json)

## Build

```
Step 1 : install workflow
git clone git@github.com:sogou/workflow.git
cd workflow
make
make install
```

```
Step 2 : install wfrest

git clone git@github.com:chanchann/wfrest.git
cd wfrest
mkdir build && cd build
cmake ..
make -j 
make install
```

## Quick start

```cpp
#include "workflow/WFFacilities.h"
#include "workflow/HttpUtil.h"
#include <csignal>
#include "HttpServer.h"
#include "HttpMsg.h"

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

    // curl -v http://ip:port/hello
    svr.Get("/hello", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("world\n");
    });
    // curl -v http://ip:port/data
    svr.Get("/data", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("Hello world\n", 12);
    });
    
    // curl -v http://ip:port/post -d 'post hello world'
    svr.Post("/post", [](const HttpReq *req, HttpResp *resp)
    {
        std::string body = req->Body();
        fprintf(stderr, "post data : %s\n", body.c_str());
    });

    if (svr.start(9001) == 0)
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
```

## API Examples

### Parameters in path

```cpp
#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"
#include "HttpMsg.h"

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

    // This handler will match /user/chanchan but will not match /user/ or /user
    // curl -v "ip:port/user/chanchan/"
    svr.Get("/user/{name}", [](HttpReq *req, HttpResp *resp)
    {
        std::string name = req->param("name");
        // resp->set_status(HttpStatusOK); // automatically
        resp->String("Hello " + name + "\n");
    });

    // wildcast/chanchan/action... (prefix)
    svr.Get("/wildcast/{name}/action*", [](HttpReq *req, HttpResp *resp)
    {
        std::string name = req->param("name");
        std::string message = name + " : path " + req->get_request_uri();

        resp->String("Hello " + message + "\n");
    });

    // request will hold the route definition
    svr.Get("/user/{name}/match*", [](HttpReq *req, HttpResp *resp)
    {
        std::string full_path = req->full_path();
        if (full_path == "/user/{name}/match*")
        {
            full_path += " match";
        } else
        {
            full_path += " dosen't match";
        }
        resp->String(full_path);
    });

    // This handler will add a new router for /user/groups.
    // Exact routes are resolved before param routes, regardless of the order they were defined.
    // Routes starting with /user/groups are never interpreted as /user/{name}/... routes
    svr.Get("/user/groups", [](HttpReq *req, HttpResp *resp)
    {
        resp->String(req->full_path());
    });

    if (svr.start(9001) == 0)
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
```

### Querystring parameters

```cpp

#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"
#include "HttpMsg.h"

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

    // The request responds to a url matching:  /query_list?username=chanchann&password=yyy
    svr.Get("/query_list", [](HttpReq *req, HttpResp *resp)
    {
        auto query_list = req->query_list();
        for(auto& query : query_list)
        {
            fprintf(stderr, "%s : %s\n", query.first.c_str(), query.second.c_str());
        }
    });

    // The request responds to a url matching:  /query?username=chanchann&password=yyy
    svr.Get("/query", [](HttpReq *req, HttpResp *resp)
    {
        std::string user_name = req->query("username");
        std::string password = req->query("password");
        std::string info = req->query("info"); // no this field
        std::string address = req->default_query("address", "china");
        resp->String(user_name + " " + password + " " + info + " " + address + "\n");
    });

    // The request responds to a url matching:  /query_has?username=chanchann&password=
    // The logic for judging whether a parameter exists is that if the parameter value is empty, the parameter is considered to exist
    // and the parameter does not exist unless the parameter is submitted.
    svr.Get("/query_has", [](HttpReq *req, HttpResp *resp)
    {
        if(req->has_query("password"))
        {
            fprintf(stderr, "has password query\n");
        }
        if(req->has_query("info"))
        {
            fprintf(stderr, "has info query\n");
        }
    });

    if (svr.start(9001) == 0)
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
```

### Post Form

```cpp
#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"
#include "HttpMsg.h"

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
        auto form_kv = req->kv;
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
        fprintf(stderr, "123\n");
        auto form_kv = req->form;
        for(auto & it : form_kv)
        {
            fprintf(stderr, "%s : %s = %s",
                                it.first.c_str(),
                                it.second.content.c_str(),
                                it.second.filename.c_str());
        }
    });

    if (svr.start(9001) == 0)
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
```

### Header

```cpp
#include "HttpServer.h"
#include "HttpMsg.h"
#include "workflow/WFFacilities.h"
#include <csignal>

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

    svr.Post("/post", [](HttpReq *req, HttpResp *resp)
    {
        std::string host = req->header("Host");
        std::string content_type = req->header("Content-Type");
        if(req->has_header("User-Agent"))
        {
            fprintf(stderr, "Has User-Agent...");
        }
        resp->String(host + " " + content_type + "\n");
    });


    if (svr.start(9001) == 0)
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
```

### Send File

```cpp
#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"
#include "HttpMsg.h"

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
    svr.mount("static");

    // single files
    svr.Get("/file1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt");
    });

    svr.Get("/file2", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("html/index.html");
    });

    svr.Get("/file3", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("/html/index.html");
    });

    svr.Get("/file4", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 0);
    });

    svr.Get("/file5", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 0, 10);
    });

    svr.Get("/file6", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 5, 10);
    });

    // multiple files
    svr.Get("/multi_files", [](const HttpReq *req, HttpResp *resp)
    {
        std::vector<std::string> file_list = {"test.txt", "todo.txt", "test1.txt"};
        resp->File(file_list);
    });

    if (svr.start(9001) == 0)
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
```

### Save File

```cpp
#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"
#include "HttpMsg.h"

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

    // curl -v -X POST "ip:port/file_write1" -F "file=@filename" -H "Content-Type: multipart/form-data"
    svr.Post("/file_write1", [](const HttpReq *req, HttpResp *resp)
    {
        std::string body = req->Body();   // multipart/form - body has boundary
        resp->Save("test.txt", std::move(body));
    });

    svr.Get("/file_write2", [](const HttpReq *req, HttpResp *resp)
    {
        std::string content = "1234567890987654321";

        resp->Save("test1.txt", std::move(content));
    });

    if (svr.start(9001) == 0)
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
```

### Upload Files 

```cpp
#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"
#include "HttpMsg.h"
#include "PathUtil.h"

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
    svr.mount("/static");

    // An expriment (Upload a file to parent dir is really dangerous.):
    // curl -v -X POST "ip:port/upload" -F "file=@demo.txt; filename=../demo.txt" -H "Content-Type: multipart/form-data"
    // Then you find the file is store in the parent dir, which is dangerous
    svr.Post("/upload", [](HttpReq *req, HttpResp *resp)
    {
        auto files = req->post_files();
        if(files.empty())
        {
            resp->set_status(HttpStatusBadRequest);
        } else
        {
            auto *file = files[0];
            // file->filename SHOULD NOT be trusted. See Content-Disposition on MDN
            // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition#directives
            // The filename is always optional and must not be used blindly by the application:
            // path information should be stripped, and conversion to the server file system rules should be done.
            fprintf(stderr, "filename : %s\n", file->filename.c_str());
            resp->Save(file->filename, std::move(file->content));
        }
    });

    // Here is the right way:
    // curl -v -X POST "ip:port/upload" -F "file=@demo.txt; filename=../demo.txt" -H "Content-Type: multipart/form-data"
    svr.Post("/upload_fix", [](HttpReq *req, HttpResp *resp)
    {
        auto files = req->post_files();
        if(files.empty())
        {
            resp->set_status(HttpStatusBadRequest);
        } else
        {
            auto *file = files[0];
            // simple solution to fix the problem above
            // This will restrict the upload file to current directory.
            resp->Save(PathUtil::base(file->filename), std::move(file->content));
        }
    });

    // upload multiple files
    // curl -X POST http://ip:port/upload_multiple \
    // -F "file1=@file1" \
    // -F "file2=@file2" \
    // -H "Content-Type: multipart/form-data"
    svr.Post("/upload_multiple", [](HttpReq *req, HttpResp *resp)
    {
        auto files = req->post_files();
        if(files.empty())
        {
            resp->set_status(HttpStatusBadRequest);
        } else
        {
            for(auto& file : files)
            {
                resp->Save(PathUtil::base(file->filename), std::move(file->content));
            }
        }
    });

    if (svr.start(9001) == 0)
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
```

### Json

```cpp
#include "workflow/WFFacilities.h"
#include <csignal>
#include "HttpServer.h"
#include "HttpMsg.h"

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

    // curl -v http://ip:port/json1
    svr.Get("/json1", [](const HttpReq *req, HttpResp *resp)
    {
        Json json;
        json["test"] = 123;
        json["json"] = "test json";
        resp->Json(json);
    });

    // curl -v http://ip:port/json2
    svr.Get("/json2", [](const HttpReq *req, HttpResp *resp)
    {
        std::string valid_text = R"(
        {
            "numbers": [1, 2, 3]
        }
        )";
        resp->Json(valid_text);
    });

    // curl -v http://ip:port/json3
    svr.Get("/json3", [](const HttpReq *req, HttpResp *resp)
    {
        std::string invalid_text = R"(
        {
            "strings": ["extra", "comma", ]
        }
        )";
        resp->Json(invalid_text);
    });

    // recieve json
    //   curl -X POST http://ip:port/json4
    //   -H 'Content-Type: application/json'
    //   -d '{"login":"my_login","password":"my_password"}'
    svr.Post("/json4", [](const HttpReq *req, HttpResp *resp)
    {
        if(req->content_type != APPLICATION_JSON)
        {
            resp->String("NOT APPLICATION_JSON");
            return;
        }
        fprintf(stderr, "Json : %s", req->json.dump(4).c_str());
    });

    if (svr.start(9001) == 0)
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
```

## ✨ Dicssussion

For more information, you can first see discussions:

**https://github.com/chanchann/wfrest/discussions**