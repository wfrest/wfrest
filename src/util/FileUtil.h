#ifndef WFREST_FILEUTIL_H_
#define WFREST_FILEUTIL_H_

#include <cstddef>
#include <string>

namespace wfrest
{
    
class FileUtil
{
public:
    static int size(const std::string &path, size_t *size);

    static bool file_exists(const std::string &path);
    
    // 创建目录，支持递归创建（类似于 mkdir -p）
    static bool create_directories(const std::string &path);
    
    // 递归删除目录
    static bool remove_directory(const std::string &path);
    
    // 创建指定大小的文件并填充随机内容
    static bool create_file_with_size(const std::string &path, size_t size_bytes);
};

} // namespace wfrest


#endif // WFREST_FILEUTIL_H_
