# 缓存静态文件

本指南演示如何使用wfrest的文件缓存系统来提高静态文件服务性能。

## 快速开始

要使用缓存提供静态文件：

```cpp
#include "wfrest/HttpServer.h"

int main()
{
    HttpServer svr;
    
    // 普通静态文件服务
    svr.Static("/static", "/path/to/files");
    
    // 缓存静态文件服务 - 对于频繁访问的文件速度更快
    svr.CachedStatic("/cached", "/path/to/files");
    
    if (svr.start(8888) == 0)
    {
        svr.wait_finish();
    }
    
    return 0;
}
```

## 在路由处理程序中使用

您也可以在路由处理程序中直接使用缓存文件功能：

```cpp
svr.GET("/style.css", [](const HttpReq *req, HttpResp *resp) {
    resp->CachedFile("/path/to/style.css");
});
```

## 缓存管理

文件缓存通过`FileCache`单例进行管理：

```cpp
#include "wfrest/FileCache.h"

// 配置缓存大小（默认为100MB）
FileCache::instance().set_max_size(50 * 1024 * 1024);  // 50MB

// 当文件更新时清除缓存
FileCache::instance().clear();

// 临时禁用缓存
FileCache::instance().disable();
```

## 工作原理

文件缓存系统：
1. 首次访问后将文件存储在内存中
2. 自动检查文件修改
3. 通过在达到大小限制时删除旧条目来管理内存使用
4. 提供显著的性能改进，特别是对于小文件

更多详情请参见[static_file_optimization.md](../static_file_optimization.md)。 