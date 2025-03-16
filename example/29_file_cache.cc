#include "wfrest/HttpServer.h"
#include "wfrest/FileCache.h"
#include <sys/stat.h>
#include <chrono>

using namespace wfrest;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <port> <directory>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    std::string directory = argv[2];

    struct stat st;
    if (stat(directory.c_str(), &st) == -1 || !S_ISDIR(st.st_mode))
    {
        fprintf(stderr, "Invalid directory: %s\n", directory.c_str());
        return 1;
    }

    HttpServer svr;

    // Configure the file cache
    FileCache& cache = FileCache::instance();
    cache.set_max_size(100 * 1024 * 1024); // 100MB cache size
    cache.enable();

    // Set up a normal static file route
    svr.Static("/static", directory.c_str());
    
    // Set up a cached static file route
    svr.CachedStatic("/cached", directory.c_str());

    // Add a route to clear the cache
    svr.GET("/clear-cache", [](const HttpReq *req, HttpResp *resp) {
        FileCache::instance().clear();
        resp->String("Cache cleared");
    });

    // Add a route to get cache info
    svr.GET("/cache-info", [](const HttpReq *req, HttpResp *resp) {
        size_t cache_size = FileCache::instance().size();
        bool enabled = FileCache::instance().is_enabled();
        
        Json json;
        json["cache_size_bytes"] = cache_size;
        json["cache_size_mb"] = cache_size / (1024.0 * 1024.0);
        json["enabled"] = enabled;
        
        resp->Json(json);
    });

    // Benchmark route to compare performance
    svr.GET("/benchmark", [&directory](const HttpReq *req, HttpResp *resp) {
        std::string filename = req->query("file");
        if (filename.empty()) {
            resp->String("Please provide a 'file' query parameter");
            return;
        }
        
        std::string filepath = directory + "/" + filename;
        
        struct stat st;
        if (stat(filepath.c_str(), &st) == -1 || !S_ISREG(st.st_mode)) {
            resp->String("File not found: " + filepath);
            return;
        }
        
        // Benchmark normal file serving
        auto start1 = std::chrono::high_resolution_clock::now();
        HttpResp resp1;
        HttpFile::send_file(filepath, 0, -1, &resp1);
        auto end1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration1 = end1 - start1;
        
        // Benchmark cached file serving
        auto start2 = std::chrono::high_resolution_clock::now();
        HttpResp resp2;
        HttpFile::send_cached_file(filepath, 0, -1, &resp2);
        auto end2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration2 = end2 - start2;
        
        // Benchmark cached file serving again (should be faster)
        auto start3 = std::chrono::high_resolution_clock::now();
        HttpResp resp3;
        HttpFile::send_cached_file(filepath, 0, -1, &resp3);
        auto end3 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration3 = end3 - start3;
        
        Json result;
        result["file"] = filename;
        result["size_bytes"] = st.st_size;
        result["normal_file_ms"] = duration1.count();
        result["cached_file_first_ms"] = duration2.count();
        result["cached_file_second_ms"] = duration3.count();
        result["speedup_vs_normal"] = duration1.count() / duration3.count();
        
        resp->Json(result);
    });

    if (svr.start(port) == 0) {
        printf("Server started on port %d\n", port);
        printf("URLs:\n");
        printf("  Normal static files: http://localhost:%d/static/...\n", port);
        printf("  Cached static files: http://localhost:%d/cached/...\n", port);
        printf("  Clear cache: http://localhost:%d/clear-cache\n", port);
        printf("  Cache info: http://localhost:%d/cache-info\n", port);
        printf("  Benchmark: http://localhost:%d/benchmark?file=yourfile.txt\n", port);
        svr.wait_finish();
    } else {
        printf("Failed to start server on port %d\n", port);
    }

    return 0;
} 