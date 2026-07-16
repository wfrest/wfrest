#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <random>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "FileUtil.h"
#include "ErrorCode.h"
#include "PathUtil.h"

using namespace wfrest;

namespace
{

const int k_directory_open_flags =
    O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;

bool ensure_directory(const std::string &path)
{
    if (mkdir(path.c_str(), 0755) == 0)
        return true;
    if (errno != EEXIST)
        return false;

    struct stat metadata;
    return stat(path.c_str(), &metadata) == 0 &&
           S_ISDIR(metadata.st_mode);
}

std::string normalize_directory_path(const std::string &path)
{
    const bool absolute = !path.empty() && path.front() == '/';
    std::vector<std::string> components;
    size_t cursor = 0;
    while (cursor < path.size())
    {
        while (cursor < path.size() && path[cursor] == '/')
            ++cursor;
        if (cursor == path.size())
            break;

        const size_t begin = cursor;
        while (cursor < path.size() && path[cursor] != '/')
            ++cursor;
        const std::string component = path.substr(begin, cursor - begin);
        if (component == ".")
            continue;
        if (component == "..")
        {
            if (!components.empty() && components.back() != "..")
                components.pop_back();
            else if (!absolute)
                components.push_back(component);
            continue;
        }
        components.push_back(component);
    }

    std::string normalized = absolute ? "/" : "";
    for (const std::string &component : components)
    {
        if (!normalized.empty() && normalized.back() != '/')
            normalized.push_back('/');
        normalized += component;
    }
    if (normalized.empty())
        return absolute ? "/" : ".";
    return normalized;
}

int open_directory_path_nofollow(const std::string &path)
{
    if (path.empty())
    {
        errno = EINVAL;
        return -1;
    }

    const bool absolute = path.front() == '/';
    int current_fd = open(absolute ? "/" : ".", k_directory_open_flags);
    if (current_fd < 0)
        return -1;

    size_t cursor = 0;
    while (cursor < path.size())
    {
        while (cursor < path.size() && path[cursor] == '/')
            ++cursor;
        if (cursor == path.size())
            break;

        const size_t begin = cursor;
        while (cursor < path.size() && path[cursor] != '/')
            ++cursor;
        const std::string component = path.substr(begin, cursor - begin);
        if (component == ".")
            continue;

        const int next_fd = openat(current_fd, component.c_str(),
                                   k_directory_open_flags);
        if (next_fd < 0)
        {
            close(current_fd);
            return -1;
        }
        if (close(current_fd) != 0)
        {
            close(next_fd);
            return -1;
        }
        current_fd = next_fd;
    }

    return current_fd;
}

bool descriptor_matches_path(const struct stat &metadata,
                             const std::string &path,
                             bool *matches)
{
    *matches = false;
    const int fd = open_directory_path_nofollow(path);
    if (fd < 0)
        return false;

    struct stat other;
    const bool inspected = fstat(fd, &other) == 0;
    const bool closed = close(fd) == 0;
    if (!inspected || !closed)
        return false;

    *matches = metadata.st_dev == other.st_dev &&
               metadata.st_ino == other.st_ino;
    return true;
}

bool remove_directory_contents(int fd, dev_t root_device)
{
    DIR *directory = fdopendir(fd);
    if (directory == nullptr)
    {
        close(fd);
        return false;
    }

    const int parent_fd = dirfd(directory);
    bool success = parent_fd >= 0;
    while (success)
    {
        errno = 0;
        struct dirent *entry = readdir(directory);
        if (entry == nullptr)
        {
            if (errno != 0)
                success = false;
            break;
        }

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        struct stat metadata;
        if (fstatat(parent_fd, entry->d_name, &metadata,
                    AT_SYMLINK_NOFOLLOW) != 0)
        {
            success = false;
            break;
        }

        if (S_ISDIR(metadata.st_mode))
        {
            if (metadata.st_dev != root_device)
            {
                success = false;
                break;
            }

            const int child_fd = openat(parent_fd, entry->d_name,
                                        k_directory_open_flags);
            if (child_fd < 0)
            {
                success = false;
                break;
            }

            struct stat opened_metadata;
            if (fstat(child_fd, &opened_metadata) != 0 ||
                opened_metadata.st_dev != metadata.st_dev ||
                opened_metadata.st_ino != metadata.st_ino)
            {
                close(child_fd);
                success = false;
                break;
            }

            if (!remove_directory_contents(child_fd, root_device) ||
                unlinkat(parent_fd, entry->d_name, AT_REMOVEDIR) != 0)
            {
                success = false;
                break;
            }
        }
        else if (unlinkat(parent_fd, entry->d_name, 0) != 0)
        {
            success = false;
            break;
        }
    }

    if (closedir(directory) != 0)
        success = false;
    return success;
}

} // namespace

int FileUtil::size(const std::string &path, size_t *size)
{
    if (size == nullptr)
        return StatusFileReadError;
    *size = 0;

    struct stat metadata;
    if (stat(path.c_str(), &metadata) != 0)
    {
        return errno == ENOENT || errno == ENOTDIR
                   ? StatusNotFound
                   : StatusFileReadError;
    }
    if (!S_ISREG(metadata.st_mode) || metadata.st_size < 0 ||
        static_cast<uintmax_t>(metadata.st_size) >
            static_cast<uintmax_t>(std::numeric_limits<size_t>::max()))
    {
        return StatusFileReadError;
    }

    *size = static_cast<size_t>(metadata.st_size);
    return StatusOK;
}

bool FileUtil::file_exists(const std::string &path)
{
    return PathUtil::is_file(path);
}

bool FileUtil::create_directories(const std::string &path)
{
    if (path.empty())
        return false;

    const bool absolute = path.front() == '/';
    std::string current_path = absolute ? "/" : "";
    bool has_component = false;
    size_t cursor = 0;
    while (cursor < path.size())
    {
        while (cursor < path.size() && path[cursor] == '/')
            ++cursor;
        if (cursor == path.size())
            break;

        const size_t begin = cursor;
        while (cursor < path.size() && path[cursor] != '/')
            ++cursor;
        const std::string component = path.substr(begin, cursor - begin);

        if (!current_path.empty() && current_path.back() != '/')
            current_path.push_back('/');
        current_path += component;
        has_component = true;
        if (!ensure_directory(current_path))
            return false;
    }

    if (!has_component)
        return absolute && ensure_directory("/");
    return true;
}

bool FileUtil::remove_directory(const std::string &path)
{
    const std::string normalized_path = normalize_directory_path(path);
    const int root_fd = open_directory_path_nofollow(path);
    if (root_fd < 0)
        return false;

    struct stat root_metadata;
    if (fstat(root_fd, &root_metadata) != 0 ||
        !S_ISDIR(root_metadata.st_mode))
    {
        close(root_fd);
        return false;
    }

    bool is_root = false;
    bool is_current = false;
    if (!descriptor_matches_path(root_metadata, "/", &is_root) ||
        !descriptor_matches_path(root_metadata, ".", &is_current) ||
        is_root || is_current)
    {
        close(root_fd);
        return false;
    }

    if (!remove_directory_contents(root_fd, root_metadata.st_dev))
        return false;
    return rmdir(normalized_path.c_str()) == 0;
}

bool FileUtil::create_file_with_size(const std::string &path,
                                     size_t size_bytes)
{
    std::ofstream file(path.c_str(), std::ios::binary);
    if (!file)
        return false;

    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(32, 126);

    std::vector<char> buffer(4096);
    size_t remaining = size_bytes;
    while (remaining > 0)
    {
        const size_t chunk_size = std::min(remaining, buffer.size());
        for (size_t i = 0; i < chunk_size; ++i)
            buffer[i] = static_cast<char>(distribution(generator));

        file.write(buffer.data(), static_cast<std::streamsize>(chunk_size));
        if (!file)
            return false;
        remaining -= chunk_size;
    }

    file.close();
    return !file.fail();
}
