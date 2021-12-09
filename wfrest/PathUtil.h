#ifndef WFREST_PATHUTIL_H_
#define WFREST_PATHUTIL_H_

#include <string>

namespace wfrest
{
class PathUtil
{
public:
    static bool isdir(const char* path);
    
public:
    static std::string concat_path(const std::string &lhs, const std::string &rhs);

    // filepath = /usr/local/image/test.jpg
    // base = test.jpg
    static std::string base(const std::string &filepath);
};
}  // namespace wfrest




#endif // WFREST_PATHUTIL_H_
