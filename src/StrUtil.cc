//
// Created by Chanchan on 11/6/21.
//

#include "StrUtil.h"

using namespace wfrest;

const std::string StrUtil::sk_pairs_ = R"({}[]()<>""''``)";

std::string StrUtil::trim_pairs(const StringPiece &str, const char *pairs)
{
    const char *lhs = str.begin();
    const char *rhs = str.begin() + str.size() - 1;
    const char *p = pairs;
    bool is_pair = false;
    while (*p != '\0' && *(p+1) != '\0')
    {
        if (*lhs == *p && *rhs == *(p + 1))
        {
            is_pair = true;
            break;
        }
        p += 2;
    }
    if(is_pair)
    {
        StringPiece res(str.begin() + 1, str.size() - 2);
        return res.as_string();
    } else
    {
        return str.as_string();
    }
}

StringPiece StrUtil::ltrim(const StringPiece &str)
{
    const char *lhs = str.begin();
    while(lhs != str.end() and std::isspace(*lhs)) lhs++;
    if(lhs == str.end()) return {};
    StringPiece res(str);
    res.remove_prefix(lhs - str.begin());
    return res;
}

StringPiece StrUtil::rtrim(const StringPiece &str)
{
    const char *rhs = str.end() - 1;
    while(rhs != str.begin() and std::isspace(*rhs)) rhs--;
    if(rhs == str.begin() and std::isspace(*rhs)) return {};
    StringPiece res(str);
    res.remove_suffix(str.end() - rhs);
    return res;
}

StringPiece StrUtil::trim(const StringPiece &str)
{
    return ltrim(rtrim(str));
}
