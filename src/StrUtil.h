//
// Created by Chanchan on 11/6/21.
//

#ifndef _STRUTIL_H_
#define _STRUTIL_H_

#include "workflow/StringUtil.h"
#include "StringPiece.h"
#include <string>

namespace wfrest
{
class StrUtil : public StringUtil
{
public:
    static StringPiece trim_pairs(const StringPiece &str, const char *pairs = sk_pairs_.c_str());

    static StringPiece ltrim(const StringPiece &str);

    static StringPiece rtrim(const StringPiece &str);

    static StringPiece trim(const StringPiece &str);

    static std::string trim(const std::string &str, const char *chars);

    template<class OutputStringType>
    static std::vector<OutputStringType> split_piece(const StringPiece &str, char sep);

private:
    static const std::string sk_pairs_;
};


template<class OutputStringType>
std::vector<OutputStringType> StrUtil::split_piece(const StringPiece &str, char sep)
{
    std::vector<OutputStringType> res;
    if (str.empty())
        return res;

    const char *p = str.begin();
    const char *cursor = p;

    while (p != str.end())
    {
        if (*p == sep)
        {
            res.emplace_back(OutputStringType(cursor, p - cursor));
            cursor = p + 1;
        }
        ++p;
    }
    res.emplace_back(OutputStringType(cursor, str.end() - cursor));
    return res;
}


}  // namespace wfrest


#endif //_STRUTIL_H_
