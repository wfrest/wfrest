#include "PathUtil.h"

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
