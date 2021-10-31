# wfrest

The c++ micro framework for building web applications based on workflow

## ⌛️ Build

```
git clone https://github.com/chanchann/wfrest.git
make
cd tutorial
make
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
    resp->File("html/index.html");
});



```