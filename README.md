[中文版入口](README_cn.md)

# ✨ wfrest: C++ Web Framework REST API

Fast🚀, efficient⌛️, and easiest💥 c++ async micro web framework based on [✨**C++ Workflow**✨](https://github.com/sogou/workflow).

[**C++ Workflow**](https://github.com/sogou/workflow) is a light-weighted C++ Parallel Computing and Asynchronous Networking Engine. 

If you need performance and good productivity, you will love ✨**wfrest**✨.

## Contents

- [✨wfrest: C++ Web Framework REST API](#wfrest:-c++-web-framework-rest-api)
    - [Discussion](#dicssussion)
    - [Contents](#contents)
    - [Build](#build)
        - [Shell](#shell)
        - [CMake](#cmake)
        - [Docker](#docker)
    - [Quick start](#quick-start)
    - [API Examples](#🎆-api-examples)
      - [Parameters in path](./docs/param_in_path.md)
      - [Query string parameters](./docs/query_param.md)
      - [Post Form](./docs/post_form.md)
      - [Header](./docs/header.md)
      - [Send File](./docs/send_file.md)
      - [Save File](./docs/save_file.md)
      - [Upload Files](./docs/upload_file.md)
      - [Json](./docs/json.md)
      - [Computing Handler](./docs/compute_handler.md)
      - [Series Handler](./docs/series_handler.md)
      - [Compression](./docs/compress.md)
      - [BluePrint](./docs/blueprint.md)
      - [Serving static files](./docs/serving_static_file.md)
      - [Cookie](./docs/cookie.md)
      - [Custom Server Configuration](./docs/config.md)
      - [Aspect-oriented programming](./docs/aop.md)
      - [Https Server](./docs/https.md)
      - [Proxy](./docs/proxy.md)
    - [MySQL](./docs/mysql.md)
    - [Redis](./docs/redis.md)
## Dicssussion

For more information, you can first see discussions:

**https://github.com/wfrest/wfrest/discussions**

## Build

### Requirement

* workflow, version v0.9.9 or newer
* Linux , like ubuntu 18.04 or newer
* Cmake
* zlib1g-dev
* libssl-dev
* libgtest-dev
* gcc and g++ or llvm + clang, tested with ubuntu 20.04

If you are on ubuntu 20.04, you may install them by command:

```bash
apt-get install build-essential cmake zlib1g-dev libssl-dev libgtest-dev -y
```

For more details, you can see here : [requirement details](./docs/requirement.md)

### Shell 

```cpp
sudo ./build.sh
```

### Cmake

```
git clone https://github.com/wfrest/wfrest
cd wfrest
mkdir build && cd build
cmake .. 
make -j  
sudo make install
```

### Docker

Use dockerfile, the Dockerfile locate `/docker` subdirectory of  root source code repository.

```
docker build -t wfrest ./docker/ubuntu/
```

If you are using `podman`, you can also build it. and tested under ubuntu 20.04

```
podman build -t wfrest ./docker/ubuntu/
```

Or you can Pull from DockerHub 

```
docker pull wfrest/wfrest
```

## Quick start

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

