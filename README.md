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
Step 2 : install spdlog

git clone git@github.com:gabime/spdlog.git
cd spdlog 
mkdir build && cd build
cmake ..
make -j
make install
```

```
Step 3 : install wfrest

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
    resp->File("html/index.html");
});
```