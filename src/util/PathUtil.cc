#include "PathUtil.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace wfrest;

std::string PathUtil::base(const std::string &filepath)
{
    std::string::size_type pos1 = filepath.find_last_not_of("/");
    if (pos1 == std::string::npos)
    {
        return "/";
    }
    std::string::size_type pos2 = filepath.find_last_of("/", pos1);
    if (pos2 == std::string::npos)
    {
        pos2 = 0;
    } else
    {
        pos2++;
    }

    return filepath.substr(pos2, pos1 - pos2 + 1);
}

std::string PathUtil::suffix(const std::string& filepath)
{
    std::string::size_type pos1 = filepath.find_last_of("/");
    if (pos1 == std::string::npos) {
        pos1 = 0;
    } else {
        pos1++;
    }
    std::string file = filepath.substr(pos1, -1);

    std::string::size_type pos2 = file.find_last_of(".");
    if (pos2 == std::string::npos) {
        return "";
    }
    return file.substr(pos2+1, -1);
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

bool PathUtil::is_dir(const std::string &path)
{
    struct stat st;
    return stat(path.c_str(), &st) >= 0 && S_ISDIR(st.st_mode);
}

bool PathUtil::is_file(const std::string &path)
{
    struct stat st;
    return stat(path.c_str(), &st) >= 0 && S_ISREG(st.st_mode);
}
