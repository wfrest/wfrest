# ✨ wfrest: C++ Web Framework REST API

Fast, efficient, and easiest c++ async micro web framework based on [C++ Workflow](https://github.com/sogou/workflow).

## 🌟 Contents

- [wfrest: C++ Web Framework REST API](#✨-wfrest:-c++-web-framework-rest-api)
    - [Discussion](#💥-dicssussion)
    - [Contents](#🌟-contents)
    - [Build](#⌛️-build)
        - [CMake](#cmake)
        - [Docker](#docker)
    - [Quick start](#🚀-quick-start)
    - [API Examples](#🎆-api-examples)
      - [Parameters in path](#parameters-in-path)
      - [Query string parameters](#querystring-parameters)
      - [Post Form](#post-form)
      - [Header](#header)
      - [Send File](#send-file)
      - [Save File](#save-file)
      - [Upload Files](#upload-files)
      - [Json](#json)
      - [Computing Handler](#computing-handler)
      - [Series Handler](#series-handler)
    - [How to use logger](#how-to-use-logger)
  
## 💥 Dicssussion

For more information, you can first see discussions:

**https://github.com/chanchann/wfrest/discussions**

## ⌛️ Build

### Cmake

```
git clone https://github.com/chanchann/wfrest.git
cd wfrest
mkdir build && cd build
cmake ..
make -j 
make install
```

### Docker

Use dockerfile

```
docker build -t wfrest .
```

Or you can Pull from DockerHub 

```
docker pull wfrest/wfrest
```

## 🚀 Quick start

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // curl -v http://ip:port/hello
    svr.GET("/hello", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("world\n");
    });
    // curl -v http://ip:port/data
    svr.GET("/data", [](const HttpReq *req, HttpResp *resp)
    {
        std::string str = "Hello world";
        resp->String(std::move(str));
    });

    // curl -v http://ip:port/post -d 'post hello world'
    svr.POST("/post", [](const HttpReq *req, HttpResp *resp)
    {
        // reference, no copy here
        std::string& body = req->body();
        fprintf(stderr, "post data : %s\n", body.c_str());
    });

    if (svr.start(8888) == 0)
    {
        getchar()
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
```

## 🎆 API Examples

### Parameters in path

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // This handler will match /user/chanchan but will not match /user/ or /user
    // curl -v "ip:port/user/chanchan/"
    svr.GET("/user/{name}", [](const HttpReq *req, HttpResp *resp)
    {
        // reference : no copy
        const std::string& name = req->param("name");
        // resp->set_status(HttpStatusOK); // automatically
        resp->String("Hello " + name + "\n");
    });

    // wildcast/chanchan/action... (prefix)
    svr.GET("/wildcast/{name}/action*", [](const HttpReq *req, HttpResp *resp)
    {
        const std::string& name = req->param("name");
        std::string message = name + " : path " + req->get_request_uri();

        resp->String("Hello " + message + "\n");
    });

    // request will hold the route definition
    svr.GET("/user/{name}/match*", [](const HttpReq *req, HttpResp *resp)
    {
        const std::string& full_path = req->full_path();
        std::string res;
        if (full_path == "/user/{name}/match*")
        {
            res = full_path + " match";
        } else
        {
            res = full_path + " dosen't match";
        }
        resp->String(res);
    });

    // This handler will add a new router for /user/groups.
    // Exact routes are resolved before param routes, regardless of the order they were defined.
    // Routes starting with /user/groups are never interpreted as /user/{name}/... routes
    svr.GET("/user/groups", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String(req->full_path());
    });

    if (svr.start(8888) == 0)
    {
        getchar()
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
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // The request responds to a url matching:  /query_list?username=chanchann&password=yyy
    svr.GET("/query_list", [](const HttpReq *req, HttpResp *resp)
    {
        const std::map<std::string, std::string>& query_list = req->query_list();
        for (auto &query: query_list)
        {
            fprintf(stderr, "%s : %s\n", query.first.c_str(), query.second.c_str());
        }
    });

    // The request responds to a url matching:  /query?username=chanchann&password=yyy
    svr.GET("/query", [](const HttpReq *req, HttpResp *resp)
    {
        const std::string& user_name = req->query("username");
        const std::string& password = req->query("password");
        const std::string& info = req->query("info"); // no this field
        const std::string& address = req->default_query("address", "china");
        resp->String(user_name + " " + password + " " + info + " " + address + "\n");
    });

    // The request responds to a url matching:  /query_has?username=chanchann&password=
    // The logic for judging whether a parameter exists is that if the parameter value is empty, the parameter is considered to exist
    // and the parameter does not exist unless the parameter is submitted.
    svr.GET("/query_has", [](const HttpReq *req, HttpResp *resp)
    {
        if (req->has_query("password"))
        {
            fprintf(stderr, "has password query\n");
        }
        if (req->has_query("info"))
        {
            fprintf(stderr, "has info query\n");
        }
    });

    if (svr.start(8888) == 0)
    {
        getchar()
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
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // Urlencoded Form
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
            fprintf(stderr, "key %s : vak %s\n", kv.first.c_str(), kv.second.c_str());
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
        const Form &form_kv = req->form();
        for (auto &it: form_kv)
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
        getchar()
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
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    svr.POST("/post", [](const HttpReq *req, HttpResp *resp)
    {
        const std::string& host = req->header("Host");
        const std::string& content_type = req->header("Content-Type");
        if (req->has_header("User-Agent"))
        {
            fprintf(stderr, "Has User-Agent...");
        }
        resp->String(host + " " + content_type + "\n");
    });


    if (svr.start(8888) == 0)
    {
        getchar()
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
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;
    svr.mount("static");

    // single files
    svr.GET("/file1", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt");
    });

    svr.GET("/file2", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("html/index.html");
    });

    svr.GET("/file3", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("/html/index.html");
    });

    svr.GET("/file4", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 0);
    });

    svr.GET("/file5", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 0, 10);
    });

    svr.GET("/file6", [](const HttpReq *req, HttpResp *resp)
    {
        resp->File("todo.txt", 5, 10);
    });

    // multiple files
    svr.GET("/multi_files", [](const HttpReq *req, HttpResp *resp)
    {
        std::vector<std::string> file_list = {"test.txt", "todo.txt", "test1.txt"};
        resp->File(file_list);
    });

    if (svr.start(8888) == 0)
    {
        getchar()
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
#include "wfrest/HttpServer.h"
using namespace wfrest;

int main()
{
    HttpServer svr;

    // curl -v -X POST "ip:port/file_write1" -F "file=@filename" -H "Content-Type: multipart/form-data"
    svr.POST("/file_write1", [](const HttpReq *req, HttpResp *resp)
    {
        std::string& body = req->body();   // multipart/form - body has boundary
        resp->Save("test.txt", std::move(body));
    });

    svr.GET("/file_write2", [](const HttpReq *req, HttpResp *resp)
    {
        std::string content = "1234567890987654321";

        resp->Save("test1.txt", std::move(content));
    });

    if (svr.start(8888) == 0)
    {
        getchar()
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
#include "wfrest/HttpServer.h"
#include "wfrest/PathUtil.h"
using namespace wfrest;

int main()
{
    HttpServer svr;
    svr.mount("/static");

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
        getchar()
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
#include "wfrest/HttpServer.h"
#include "wfrest/json.hpp"
using namespace wfrest;
using Json = nlohmann::json;

int main()
{
    HttpServer svr;

    // curl -v http://ip:port/json1
    svr.GET("/json1", [](const HttpReq *req, HttpResp *resp)
    {
        Json json;
        json["test"] = 123;
        json["json"] = "test json";
        resp->Json(json);
    });

    // curl -v http://ip:port/json2
    svr.GET("/json2", [](const HttpReq *req, HttpResp *resp)
    {
        std::string valid_text = R"(
        {
            "numbers": [1, 2, 3]
        }
        )";
        resp->Json(valid_text);
    });

    // curl -v http://ip:port/json3
    svr.GET("/json3", [](const HttpReq *req, HttpResp *resp)
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
    svr.POST("/json4", [](const HttpReq *req, HttpResp *resp)
    {
        if (req->content_type() != APPLICATION_JSON)
        {
            resp->String("NOT APPLICATION_JSON");
            return;
        }
        fprintf(stderr, "Json : %s", req->json().dump(4).c_str());
    });

    if (svr.start(8888) == 0)
    {
        getchar()
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
```

### Computing Handler

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest

void Fibonacci(int n, HttpResp *resp)
{
    unsigned long long x = 0, y = 1;
    if (n <= 0 || n > 94)
    {
        fprintf(stderr, "invalid parameter");
        return;
    }
    for (int i = 2; i < n; i++)
    {
        y = x + y;
        x = y - x;
    }
    if (n == 1)
        y = 0;
    resp->String("fib(" + std::to_string(n) + ") is : " + std::to_string(y) + "\n");
}

int main()
{
    HttpServer svr;
    // Second parameter means this computing queue id is 1
    // Then this handler become a computing task
    // curl -v http://ip:port/compute_task?num=20
    svr.GET("/compute_task", 1, [](const HttpReq *req, HttpResp *resp)
    {
        int num = std::stoi(req->query("num"));
        Fibonacci(num, resp);
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

### Series Handler

```cpp
#include "wfrest/HttpServer.h"
using namespace wfrest

int main()
{
    HttpServer svr;

    svr.GET("/series", [](const HttpReq *req, HttpResp *resp, SeriesWork* series)
    {
        auto *timer = WFTaskFactory::create_timer_task(5000000, [](WFTimerTask *) {
            printf("timer task complete(5s).\n");
        });

        series->push_back(timer);
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

## How to use logger

```cpp
#include "wfrest/Logger.h"
using namespace wfrest

int main()
{
    // set the logger config
    LoggerSettings settings = LOGGER_SETTINGS_DEFAULT;
    settings.level = LogLevel::TRACE;
    settings.log_in_file = true;
    LOGGER(&settings);

    int i = 1;
    LOG_DEBUG << (float)3.14;
    LOG_DEBUG << (const char)'8';
    LOG_DEBUG << &i;
    LOG_DEBUG << wfrest::Fmt("%.3f", 3.1415926);
    LOG_DEBUG << "debug";
    LOG_TRACE << "trace";
    LOG_INFO << "info";
    LOG_WARN << "warning";

    FILE *fp = fopen("/not_exist_file", "rb");
    if (fp == nullptr)
    {
        LOG_SYSERR << "syserr log!";
    }
    LOG_DEBUG  << abc << 123.345 << "chanchan" << '\n'
               << std::string("name");
    return 0;
}
```

All the configure fields are:

```cpp
struct LoggerSettings
{
    LogLevel level;    
    bool log_in_console;    
    bool log_in_file;
    const char *file_path;
    const char *file_base_name;
    const char *file_extension;
    uint64_t roll_size;
    std::chrono::seconds flush_interval;
};
```

Default configure :

```cpp
static constexpr struct LoggerSettings LOGGER_SETTINGS_DEFAULT =
{
    .level = LogLevel::INFO,
    .log_in_console = true,
    .log_in_file = false,
    .file_path = "./",
    .file_base_name = "wfrest",
    .file_extension = ".log",
    .roll_size = 20 * 1024 * 1024,
    .flush_interval = std::chrono::seconds(3),
};
```

Sample Output

```
2021-11-30 22:36:21.422271 822380  [ERROR]  [logger_test.cc:84] No such file or directory (errno=2) syserr log
```

