# wfrest

The c++ micro framework for building web applications based on workflow

## ⌛️ Build

```
Step 1 : install workflow
git clne git@github.com:sogou/workflow.git
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

## ⚡️ Simple examples

```cpp
svr.Get("/hello", [](const HttpReq *req, HttpResp *resp)
{
    resp->send("world\n");
});

svr.Get("/data", [](const HttpReq *req, HttpResp *resp)
{
    resp->send_no_copy("Hello world\n", 12);
});

svr.Get("/api/{name}", [](HttpReq *req, HttpResp *resp)
{
    std::string name = req->get("name");
    resp->send(name+"\n");
});

svr.Get("/json", [](const HttpReq *req, HttpResp *resp)
{
    json js;
    js["test"] = 123;
    js["json"] = "test json";
    resp->send(js.dump());
});

svr.Get("/html/index.html", [](const HttpReq *req, HttpResp *resp)
{
    resp->file("html/index.html");
});

svr.Post("/post", [](const HttpReq *req, HttpResp *resp)
{
    std::string body = req->body();
    fprintf(stderr, "post data : %s\n", body.c_str());
});
```

## ✨ Dicssussion

For more information, you can first see discussions:

**https://github.com/chanchann/wfrest/discussions**