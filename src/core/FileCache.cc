#include "FileCache.h"

#include <algorithm>
#include <cstdint>
#include <sys/stat.h>

namespace wfrest
{

namespace
{

bool snapshot_is_current(const std::string& path,
                         const std::shared_ptr<CachedFile>& snapshot)
{
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) != 0 ||
        !S_ISREG(file_stat.st_mode) || file_stat.st_size < 0)
    {
        return false;
    }

    return static_cast<uintmax_t>(file_stat.st_size) ==
               static_cast<uintmax_t>(snapshot->size) &&
           file_stat.st_mtime == snapshot->last_modified;
}

} // namespace

bool FileCache::get_file(const std::string& path, std::string& content,
                         size_t start, size_t end)
{
    if (!enabled_.load(std::memory_order_acquire))
        return false;

    std::shared_ptr<CachedFile> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!enabled_.load(std::memory_order_relaxed))
            return false;

        auto it = cache_.find(path);
        if (it == cache_.end())
            return false;
        snapshot = it->second;
    }

    if (!snapshot_is_current(path, snapshot))
    {
        evict_if_same(path, snapshot);
        return false;
    }

    if (end == static_cast<size_t>(-1) || end >= snapshot->content.size())
        end = snapshot->content.size();

    start = std::min(start, end);
    content.assign(snapshot->content, start, end - start);
    return true;
}

void FileCache::add_file(const std::string& path, const std::string& content,
                         std::time_t last_modified)
{
    if (!enabled_.load(std::memory_order_acquire))
        return;

    auto snapshot = std::make_shared<CachedFile>();
    snapshot->content = content;
    snapshot->last_modified = last_modified;
    snapshot->size = content.size();

    std::lock_guard<std::mutex> lock(mutex_);
    if (!enabled_.load(std::memory_order_relaxed))
        return;

    auto it = cache_.find(path);
    if (it != cache_.end())
    {
        current_size_ -= it->second->size;
        cache_.erase(it);
    }

    if (snapshot->size > max_cache_size_)
        return;

    manage_cache_size(snapshot->size);
    cache_[path] = std::move(snapshot);
    current_size_ += content.size();
}

bool FileCache::is_valid(const std::string& path)
{
    if (!enabled_.load(std::memory_order_acquire))
        return false;

    std::shared_ptr<CachedFile> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!enabled_.load(std::memory_order_relaxed))
            return false;

        auto it = cache_.find(path);
        if (it == cache_.end())
            return false;
        snapshot = it->second;
    }

    if (snapshot_is_current(path, snapshot))
        return true;

    evict_if_same(path, snapshot);
    return false;
}

void FileCache::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    current_size_ = 0;
}

void FileCache::set_max_size(size_t max_size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    max_cache_size_ = max_size;
    manage_cache_size(0);
}

void FileCache::evict_if_same(const std::string& path,
                              const std::shared_ptr<CachedFile>& snapshot)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(path);
    if (it != cache_.end() && it->second == snapshot)
    {
        current_size_ -= it->second->size;
        cache_.erase(it);
    }
}

void FileCache::manage_cache_size(size_t incoming_size)
{
    if (incoming_size > max_cache_size_)
        return;

    const size_t available_before_insert = max_cache_size_ - incoming_size;
    const size_t retention_target = max_cache_size_ - max_cache_size_ / 4;
    const size_t target_size = std::min(available_before_insert,
                                        retention_target);

    auto it = cache_.begin();
    while (current_size_ > target_size && it != cache_.end())
    {
        current_size_ -= it->second->size;
        it = cache_.erase(it);
    }
}

} // namespace wfrest
