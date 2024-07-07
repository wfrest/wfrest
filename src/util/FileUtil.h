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
};

} // namespace wfrest


#endif // WFREST_FILEUTIL_H_
