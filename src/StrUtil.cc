//
// Created by Chanchan on 11/6/21.
//

#include "StrUtil.h"

using namespace wfrest;

std::string StrUtil::trim_pairs(const std::string &str, const char *pairs)
{
    const char* s = str.c_str();
    const char* e = str.c_str() + str.size() - 1;
    const char* p = pairs;
    bool is_pair = false;
    while (*p != '\0' && *(p+1) != '\0') {
        if (*s == *p && *e == *(p+1)) {
            is_pair = true;
            break;
        }
        p += 2;
    }
    return is_pair ? str.substr(1, str.size()-2) : str;
}

std::string StrUtil::trim(const std::string &str, const char *chars)
{
    std::string::size_type pos1 = str.find_first_not_of(chars);
    if (pos1 == std::string::npos)   return "";

    std::string::size_type pos2 = str.find_last_not_of(chars);
    return str.substr(pos1, pos2-pos1+1);
}
