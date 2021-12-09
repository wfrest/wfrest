#include "wfrest/PathUtil.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace wfrest;

std::string PathUtil::base(const std::string &filepath)
{
    std::string::size_type pos1 = filepath.find_last_not_of("/\\");
    if (pos1 == std::string::npos)
    {
        return "/";
    }
    std::string::size_type pos2 = filepath.find_last_of("/\\", pos1);
    if (pos2 == std::string::npos)
    {
        pos2 = 0;
    } else
    {
        pos2++;
    }

    return filepath.substr(pos2, pos1 - pos2 + 1);
}

std::string PathUtil::concat_path(const std::string &lhs, const std::string &rhs)
{
    std::string res;
    // /v1/ /v2
    // remove one '/' -> /v1/v2
    if(lhs.back() == '/' && rhs.front() == '/')
    {
        res = lhs.substr(0, lhs.size() - 1) + rhs;
    } else if(lhs.back() != '/' && rhs.front() != '/')
    {
        res = lhs + "/" + rhs;
    } else 
    {
        res = lhs + rhs;
    }
    return res;
}

bool PathUtil::isdir(const char* path)
{
    struct stat path_stat;
    if (-1 == stat(path, &path_stat))
        return false;
    return S_ISDIR(path_stat.st_mode);
}
