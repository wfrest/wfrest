#include "StrUtil.h"

using namespace wfrest;

const std::string StrUtil::k_pairs_ = R"({}[]()<>""''``)";

StringPiece StrUtil::trim_pairs(const StringPiece &str, const char *pairs)
{
    const char *lhs = str.begin();
    const char *rhs = str.begin() + str.size() - 1;
    const char *p = pairs;
    bool is_pair = false;
    while (*p != '\0' && *(p + 1) != '\0')
    {
        if (*lhs == *p && *rhs == *(p + 1))
        {
            is_pair = true;
            break;
        }
        p += 2;
    }
    return is_pair ? StringPiece(str.begin() + 1, str.size() - 2) : str;
}

StringPiece StrUtil::ltrim(const StringPiece &str)
{
    const char *lhs = str.begin();
    while (lhs != str.end() and std::isspace(*lhs)) lhs++;
    if (lhs == str.end()) return {};
    StringPiece res(str);
    res.remove_prefix(lhs - str.begin());
    return res;
}

StringPiece StrUtil::rtrim(const StringPiece &str)
{
    if (str.empty()) return str;
    const char *rhs = str.end() - 1;
    while (rhs != str.begin() and std::isspace(*rhs)) rhs--;
    if (rhs == str.begin() and std::isspace(*rhs)) return {};
    StringPiece res(str.begin(), rhs - str.begin() + 1);
    return res;
}

StringPiece StrUtil::trim(const StringPiece &str)
{
    return ltrim(rtrim(str));
}

std::string StrUtil::trim(const std::string &str, const char *chars)
{
    std::string::size_type pos1 = str.find_first_not_of(chars);
    if (pos1 == std::string::npos) return "";

    std::string::size_type pos2 = str.find_last_not_of(chars);
    return str.substr(pos1, pos2 - pos1 + 1);
}