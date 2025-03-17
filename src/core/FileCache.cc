#include "FileCache.h"
#include "PathUtil.h"
#include "FileUtil.h"
#include "ErrorCode.h"

#include <fstream>
#include <sys/stat.h>
#include <algorithm>
#include <vector>

namespace wfrest
{

bool FileCache::get_file(const std::string& path, std::string& content, size_t start, size_t end)
{
    // First check if caching is enabled without locking
    if (!enabled_)
        return false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!enabled_)
        return false;

    auto it = cache_.find(path);
    if (it != cache_.end()) {
        // Check if file has been modified
        std::time_t current_mod_time = get_file_modification_time(path);
        if (current_mod_time <= 0 || current_mod_time > it->second->last_modified) {
            // File has been modified or doesn't exist anymore
            return false;
        }

        // File is in cache and up to date
        if (end == (size_t)-1 || end >= it->second->content.size()) {
            end = it->second->content.size();
        }
            
        start = std::min(start, end);
        content = it->second->content.substr(start, end - start);
        return true;
    }
    
    return false;
}

void FileCache::add_file(const std::string& path, const std::string& content, std::time_t last_modified)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!enabled_)
        return;
        
    // Check if we need to make room in the cache
    if (current_size_ + content.size() > max_cache_size_) {
        manage_cache_size();
    }
    
    // If file already in cache, update it
    auto it = cache_.find(path);
    if (it != cache_.end()) {
        current_size_ -= it->second->content.size();
        it->second->content = content;
        it->second->last_modified = last_modified;
        it->second->size = content.size();
        current_size_ += content.size();
    } else {
        // Add new file to cache
        auto cached_file = std::make_shared<CachedFile>();
        cached_file->content = content;
        cached_file->last_modified = last_modified;
        cached_file->size = content.size();
        
        cache_[path] = cached_file;
        current_size_ += content.size();
    }
}

bool FileCache::is_valid(const std::string& path)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!enabled_)
        return false;
        
    auto it = cache_.find(path);
    if (it == cache_.end()) {
        return false;
    }
    
    std::time_t current_mod_time = get_file_modification_time(path);
    return (current_mod_time > 0 && current_mod_time <= it->second->last_modified);
}

void FileCache::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    current_size_ = 0;
}

std::time_t FileCache::get_file_modification_time(const std::string& path)
{
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) != 0) {
        return 0; // Error getting file stats
    }
    return file_stat.st_mtime;
}

void FileCache::manage_cache_size()
{
    // Simple approach: remove random items until we have enough space
    // A more sophisticated approach would use LRU or similar policy
    
    // We want to reduce to 75% of max size to avoid frequent cleanup
    size_t target_size = max_cache_size_ * 3 / 4;
    
    // While we're over the target, remove items
    std::vector<std::string> paths_to_remove;
    
    for (auto& entry : cache_) {
        paths_to_remove.push_back(entry.first);
        current_size_ -= entry.second->size;
        
        if (current_size_ <= target_size) {
            break;
        }
    }
    
    for (const auto& path : paths_to_remove) {
        cache_.erase(path);
    }
}

} // namespace wfrest 
