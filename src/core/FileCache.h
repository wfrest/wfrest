#ifndef WFREST_FILECACHE_H_
#define WFREST_FILECACHE_H_

#include <string>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <memory>
#include <ctime>

namespace wfrest
{

struct CachedFile {
    std::string content;
    std::time_t last_modified;
    std::size_t size;
};

class FileCache {
public:
    static FileCache& instance() {
        static FileCache cache;
        return cache;
    }

    // Get file from cache or load it if not available
    bool get_file(const std::string& path, std::string& content, size_t start = 0, size_t end = (size_t)-1);
    
    // Add file to cache
    void add_file(const std::string& path, const std::string& content, std::time_t last_modified);
    
    // Check if file exists in cache and is up-to-date
    bool is_valid(const std::string& path);
    
    // Clear all cached files
    void clear();
    
    // Set maximum cache size
    void set_max_size(size_t max_size);
    
    // Get current cache size
    size_t size() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return current_size_; 
    }

    // Enable/Disable caching
    void enable() { 
        std::lock_guard<std::mutex> lock(mutex_);
        enabled_.store(true, std::memory_order_release);
    }
    
    void disable() { 
        std::lock_guard<std::mutex> lock(mutex_);
        enabled_.store(false, std::memory_order_release);
    }
    
    bool is_enabled() const {
        return enabled_.load(std::memory_order_acquire);
    }

private:
    FileCache() : max_cache_size_(100 * 1024 * 1024), current_size_(0), enabled_(true) {}
    ~FileCache() = default;
    
    // Make non-copyable
    FileCache(const FileCache&) = delete;
    FileCache& operator=(const FileCache&) = delete;
    
    void evict_if_same(const std::string& path,
                       const std::shared_ptr<CachedFile>& snapshot);

    // Remove entries until an incoming object can fit within the size bound.
    void manage_cache_size(size_t incoming_size);

private:
    std::unordered_map<std::string, std::shared_ptr<CachedFile>> cache_;
    mutable std::mutex mutex_; // mutex for thread safety
    size_t max_cache_size_;
    size_t current_size_;
    std::atomic<bool> enabled_;
};

} // namespace wfrest

#endif // WFREST_FILECACHE_H_
