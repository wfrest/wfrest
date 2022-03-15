#include "HttpDef.h"
#include <cstring>

using namespace wfrest;

namespace 
{

int strstartswith(const char *str, const char *start)
{
    while (*str && *start && *str == *start)
    {
        ++str;
        ++start;
    }
    return *start == '\0';
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
#define XX(name, string, suffix) \
    if (strstartswith(content_type_str.c_str(), #string)) { \
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
