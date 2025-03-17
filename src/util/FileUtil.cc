#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <vector>
#include <random>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include "FileUtil.h"
#include "ErrorCode.h"
#include "PathUtil.h"

using namespace wfrest;

int FileUtil::size(const std::string &path, size_t *size)
{
    // https://linux.die.net/man/2/stat
    struct stat st;
    memset(&st, 0, sizeof st);
    int ret = stat(path.c_str(), &st);
    int status = StatusOK;
    if(ret == -1)
    {
        *size = 0;
        status = StatusNotFound;
    } else
    {
        *size = st.st_size;
    }
    return status;
}

bool FileUtil::file_exists(const std::string &path)
{
    return PathUtil::is_file(path);
}

bool FileUtil::create_directories(const std::string &path)
{
    std::string current_path;
    std::string delimiter = "/";
    size_t pos = 0;
    std::string token;
    std::string str = path;
    
    while ((pos = str.find(delimiter)) != std::string::npos) {
        token = str.substr(0, pos);
        current_path += token + delimiter;
        if (!current_path.empty()) {
            // 创建目录，忽略已存在的情况
            mkdir(current_path.c_str(), 0755);
        }
        str.erase(0, pos + delimiter.length());
    }
    
    if (!str.empty()) {
        current_path += str;
        return mkdir(current_path.c_str(), 0755) == 0 || errno == EEXIST;
    }
    
    return true;
}

bool FileUtil::remove_directory(const std::string &path)
{
    DIR* d = opendir(path.c_str());
    if (!d) {
        return false;
    }
    
    struct dirent* dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
            continue;
        }
        
        std::string full_path = path + "/" + dir->d_name;
        struct stat s;
        if (stat(full_path.c_str(), &s) == 0) {
            if (S_ISDIR(s.st_mode)) {
                remove_directory(full_path);
            } else {
                unlink(full_path.c_str());
            }
        }
    }
    
    closedir(d);
    return rmdir(path.c_str()) == 0;
}

bool FileUtil::create_file_with_size(const std::string &path, size_t size_bytes)
{
    std::ofstream file(path.c_str(), std::ios::binary);
    if (!file) {
        return false;
    }

    // 生成随机内容
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(32, 126); // 可打印ASCII字符

    std::vector<char> buffer(4096);
    size_t remaining = size_bytes;

    while (remaining > 0) {
        size_t chunk_size = std::min(remaining, buffer.size());
        for (size_t i = 0; i < chunk_size; ++i) {
            buffer[i] = static_cast<char>(distrib(gen));
        }
        file.write(buffer.data(), chunk_size);
        remaining -= chunk_size;
    }

    file.close();
    return true;
}
