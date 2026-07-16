#include "HttpDef.h"
#include <cstring>

using namespace wfrest;

namespace 
{

bool is_ows(char ch)
{
    return ch == ' ' || ch == '\t';
}

unsigned char ascii_lower(unsigned char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return static_cast<unsigned char>(ch + ('a' - 'A'));

    return ch;
}

bool ascii_iequal(const char *lhs, size_t lhs_len, const char *rhs)
{
    const size_t rhs_len = strlen(rhs);
    if (lhs_len != rhs_len)
        return false;

    for (size_t i = 0; i < lhs_len; ++i)
    {
        if (ascii_lower(static_cast<unsigned char>(lhs[i])) !=
            ascii_lower(static_cast<unsigned char>(rhs[i])))
        {
            return false;
        }
    }

    return true;
}

} // namespace 


std::string ContentType::to_str(enum http_content_type type)
{
    switch (type)
    {
#define XX(name, string, suffix) case name: return #string;
        HTTP_CONTENT_TYPE_MAP(XX)
#undef XX
        default:
            return "<unknown>";
    }
}

enum http_content_type ContentType::to_enum(const std::string &content_type_str)
{
    if (content_type_str.empty())
    {
        return CONTENT_TYPE_NONE;
    }

    size_t begin = 0;
    size_t end = content_type_str.find(';');
    if (end == std::string::npos)
        end = content_type_str.size();

    while (begin < end && is_ows(content_type_str[begin]))
        ++begin;
    while (end > begin && is_ows(content_type_str[end - 1]))
        --end;

    if (begin == end)
    {
        for (char ch : content_type_str)
        {
            if (!is_ows(ch))
                return CONTENT_TYPE_UNDEFINED;
        }
        return CONTENT_TYPE_NONE;
    }

#define XX(name, string, suffix) \
    if (ascii_iequal(content_type_str.data() + begin, end - begin, #string)) { \
        return name; \
    }
    HTTP_CONTENT_TYPE_MAP(XX)
#undef XX
    return CONTENT_TYPE_UNDEFINED;
}

std::string ContentType::to_str_by_suffix(const std::string &str)
{
    if (str.empty())
    {
        return "";
    }
#define XX(name, string, suffix) \
    if (str == #suffix) { \
        return #string; \
    }
    HTTP_CONTENT_TYPE_MAP(XX)
#undef XX
    return "";
}

enum http_content_type ContentType::to_enum_by_suffix(const std::string &str)
{
    if (str.empty()) {
        return CONTENT_TYPE_NONE;
    }
#define XX(name, string, suffix) \
    if (str == #suffix) { \
        return name; \
    }
    HTTP_CONTENT_TYPE_MAP(XX)
#undef XX
    return CONTENT_TYPE_UNDEFINED;
}
