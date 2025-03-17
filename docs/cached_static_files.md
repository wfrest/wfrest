# Cached Static Files

This guide demonstrates how to use wfrest's file caching system to improve static file serving performance.

## Quick Start

To serve static files with caching:

```cpp
#include "wfrest/HttpServer.h"

int main()
{
    HttpServer svr;
    
    // Normal static file serving
    svr.Static("/static", "/path/to/files");
    
    // Cached static file serving - much faster for frequently accessed files
    svr.CachedStatic("/cached", "/path/to/files");
    
    if (svr.start(8888) == 0)
    {
        svr.wait_finish();
    }
    
    return 0;
}
```

## Using in Route Handlers

You can also use the cached file functionality directly in route handlers:

```cpp
svr.GET("/style.css", [](const HttpReq *req, HttpResp *resp) {
    resp->CachedFile("/path/to/style.css");
});
```

## Cache Management

The file cache is managed through the `FileCache` singleton:

```cpp
#include "wfrest/FileCache.h"

// Configure cache size (default is 100MB)
FileCache::instance().set_max_size(50 * 1024 * 1024);  // 50MB

// Clear the cache when files are updated
FileCache::instance().clear();

// Temporarily disable caching
FileCache::instance().disable();
```

## How It Works

The file caching system:
1. Stores files in memory after first access
2. Automatically checks for file modifications 
3. Manages memory usage by removing old entries when reaching size limits
4. Provides significant performance improvements, especially for small files

See [static_file_optimization.md](./static_file_optimization.md) for more details. 