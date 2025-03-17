# 静态文件服务优化

## 背景

静态文件服务是Web服务器的常见功能。默认情况下，wfrest使用异步文件I/O，这对于大文件和并发操作非常出色，但对于CSS、JavaScript或小图像等小型、频繁访问的文件，会增加额外的开销。

正如[issue #272](https://github.com/wfrest/wfrest/issues/272)中指出的，对于小文件，以这种方式提供静态文件的速度可能比常规Apache配置慢4倍。

## 文件缓存服务

为了解决这个性能问题，wfrest现在提供了一个文件缓存系统，该系统在首次访问后将文件存储在内存中，显著提高了频繁访问文件的响应时间。

### 在处理程序中使用缓存文件

```cpp
// 在处理程序中直接使用CachedFile方法
svr.GET("/css", [](const HttpReq *req, HttpResp *resp) {
    resp->CachedFile("/path/to/styles.css");
});
```

与常规的`File()`方法一样，`CachedFile()`也支持指定范围：

```cpp
// 提供文件的一部分
resp->CachedFile("/path/to/file.txt", 100, 500);  // 第100-500字节
```

### 使用缓存提供静态目录

对于提供整个目录，使用`CachedStatic`方法：

```cpp
HttpServer svr;

// 传统静态文件服务
svr.Static("/static", "/var/www/files");

// 缓存静态文件服务
svr.CachedStatic("/cached", "/var/www/files");
```

这设置了路由，使用缓存系统从指定目录提供文件。

## 缓存管理

文件缓存通过`FileCache`单例进行管理：

```cpp
#include "wfrest/FileCache.h"

// 获取缓存实例
FileCache& cache = FileCache::instance();

// 配置缓存大小（默认为100MB）
cache.set_max_size(200 * 1024 * 1024);  // 200MB

// 清除缓存（当文件更新时很有用）
cache.clear();

// 如果需要，禁用缓存
cache.disable();

// 重新启用缓存
cache.enable();

// 获取当前缓存大小
size_t current_size = cache.size();
```

## 缓存工作原理

1. 当通过`CachedFile()`首次请求文件时，它会从磁盘读取并存储在内存中
2. 后续请求会检查文件是否在缓存中并且是最新的（跟踪修改时间）
3. 如果文件已被修改，它会从磁盘重新读取并更新缓存
4. 当缓存达到其大小限制时，会自动删除较旧的项目

## 性能考虑

- 文件缓存最适合频繁访问且不经常更改的文件
- 对于非常大的文件，标准的`File()`方法可能更合适
- 缓存会检查文件修改时间，因此更新的文件会自动刷新
- 缓存静态文件服务对于小文件可以明显更快（3-4倍）

## 示例

`example/file_cache.cc`中提供了一个完整示例，演示了：

- 设置具有缓存和非缓存路由的服务器
- 对比性能差异
- 管理缓存

运行示例：

```bash
./file_cache 8080 /path/to/static/files
```

然后访问：
- `http://localhost:8080/static/...` - 标准文件服务
- `http://localhost:8080/cached/...` - 缓存文件服务
- `http://localhost:8080/benchmark?file=example.css` - 性能比较
- `http://localhost:8080/cache-info` - 缓存统计
- `http://localhost:8080/clear-cache` - 清除缓存 