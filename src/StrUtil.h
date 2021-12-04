#ifndef WFREST_STRUTIL_H_
#define WFREST_STRUTIL_H_

#include "workflow/StringUtil.h"
#include "StringPiece.h"
#include <string>

namespace wfrest
{
class StrUtil : public StringUtil
{
public:
    static StringPiece trim_pairs(const StringPiece &str, const char *pairs = k_pairs_.c_str());

    static StringPiece ltrim(const StringPiece &str);

    static StringPiece rtrim(const StringPiece &str);

    static StringPiece trim(const StringPiece &str);

    template<class OutputStringType>
    static std::vector<OutputStringType> split_piece(const StringPiece &str, char sep);

private:
    static const std::string k_pairs_;
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

// std::map<std::string, std::string, MapStringCaseLess>
class MapStringCaseLess {
public:
    bool operator()(const std::string& lhs, const std::string& rhs) const {
        return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
    }
};

// std::unordered_map<std::string, std::string
//    , UnorderMapStringCaseLess::hash
//    , UnorderMapStringCaseLess::comp>
class UnorderMapStringCaseLess {
public:
    struct Comp {
        bool operator() (const std::string& lhs, const std::string& rhs) const {
            return strcasecmp(lhs.c_str(), rhs.c_str()) == 0;
        }
    };
    struct Hash {
        std::size_t operator() (std::string str) const {
            for (std::size_t index = 0; index < str.size(); ++index) {
                auto ch = static_cast<unsigned char>(str[index]);
                str[index] = static_cast<unsigned char>(std::tolower(ch));
            }
            return std::hash<std::string>{}(str);
        }
    };
};

}  // namespace wfrest


#endif // WFREST_STRUTIL_H_
