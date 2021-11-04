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
svr.Get("/hello", [](const HttpReq* req, HttpResp* resp) {
    resp->String("world\n");
});

svr.Get("/data", [](const HttpReq* req, HttpResp* resp) {
    resp->Data("Hello world\n", 12 /* true */);
});

svr.Get("/json", [](const HttpReq* req, HttpResp* resp) {
    json js;
    js["test"] = 123;
    js["json"] = "test json";
    resp->String(js.dump());
});

svr.Get("/html/index.html", [](const HttpReq* req, HttpResp* resp) {
    resp->file("html/index.html");
});

svr.Post("/post", [](const HttpReq* req, HttpResp* resp){
    const char *body;
    size_t body_len = 0;
    req->body(&body, &body_len);
    fprintf(stderr, "post data : %s\n", body);
});

svr.Post("/enlen", [](const HttpReq* req, HttpResp* resp){
    protocol::HttpHeaderCursor cursor(req);
    std::string content_type;
    cursor.find("Content-Type", content_type);
    if(content_type != "application/x-www-form-urlencoded")
    {
        resp->set_status(HttpStatusBadRequest);
        return;
    }
    const char *body;
    size_t body_len = 0;
    req->body(&body, &body_len);
    fprintf(stderr, "post data : %s\n", body);
});
```

## ✨ Dicssussion

For more information, you can first see discussions:

**https://github.com/chanchann/wfrest/discussions**