# Static File Serving Optimization

## Background

Static file serving is a common functionality in web servers. By default, wfrest uses asynchronous file I/O, which while excellent for large files and concurrent operations, adds overhead for small, frequently accessed files like CSS, JavaScript, or small images.

As identified in [issue #272](https://github.com/wfrest/wfrest/issues/272), serving static files this way can be up to 4 times slower than a regular Apache setup for small files.

## Cached File Serving

To address this performance issue, wfrest now provides a file caching system that stores files in memory after the first access, significantly improving response times for frequently accessed files.

### Using Cached File in Handlers

```cpp
// Using the CachedFile method directly in a handler
svr.GET("/css", [](const HttpReq *req, HttpResp *resp) {
    resp->CachedFile("/path/to/styles.css");
});
```

Just like the regular `File()` method, `CachedFile()` also supports specifying ranges:

```cpp
// Serve a portion of a file
resp->CachedFile("/path/to/file.txt", 100, 500);  // Bytes 100-500
```

### Serving Static Directories with Caching

For serving entire directories, use the `CachedStatic` method:

```cpp
HttpServer svr;

// Traditional static file serving
svr.Static("/static", "/var/www/files");

// Cached static file serving
svr.CachedStatic("/cached", "/var/www/files");
```

This sets up routes to serve files from the specified directory using the cache system.

## Cache Management

The file cache is managed through the `FileCache` singleton:

```cpp
#include "wfrest/FileCache.h"

// Get the cache instance
FileCache& cache = FileCache::instance();

// Configure cache size (default is 100MB)
cache.set_max_size(200 * 1024 * 1024);  // 200MB

// Clear the cache (useful when files are updated)
cache.clear();

// Disable caching if needed
cache.disable();

// Re-enable caching
cache.enable();

// Get current cache size
size_t current_size = cache.size();
```

## How Caching Works

1. When a file is first requested via `CachedFile()`, it is read from disk and stored in memory
2. Subsequent requests check if the file is in cache and up-to-date (modification time is tracked)
3. If the file has been modified, it is re-read from disk and the cache is updated
4. When the cache reaches its size limit, older items are removed automatically

## Performance Considerations

- File caching works best for frequently accessed files that don't change often
- For very large files, the standard `File()` method may be more appropriate
- The cache checks file modification times, so updated files are automatically refreshed
- Cached static file serving can be significantly faster (3-4x) than regular serving for small files

## Example

A complete example is available in `example/file_cache.cc` which demonstrates:

- Setting up a server with both cached and non-cached routes
- Benchmarking the performance difference
- Managing the cache

Run the example with:

```bash
./file_cache 8080 /path/to/static/files
```

Then visit:
- `http://localhost:8080/static/...` - Standard file serving
- `http://localhost:8080/cached/...` - Cached file serving
- `http://localhost:8080/benchmark?file=example.css` - Performance comparison
- `http://localhost:8080/cache-info` - Cache statistics
- `http://localhost:8080/clear-cache` - Clear the cache 