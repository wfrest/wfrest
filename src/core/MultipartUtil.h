#ifndef WFREST_MULTIPARTUTIL_H_
#define WFREST_MULTIPARTUTIL_H_

#include <string>

namespace wfrest
{
namespace detail
{

inline bool is_multipart_boundary_char(unsigned char ch)
{
    if ((ch >= '0' && ch <= '9') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= 'a' && ch <= 'z'))
    {
        return true;
    }

    switch (ch)
    {
        case ' ':
        case '\'':
        case '(':
        case ')':
        case '+':
        case '_':
        case ',':
        case '-':
        case '.':
        case '/':
        case ':':
        case '=':
        case '?':
            return true;
        default:
            return false;
    }
}

inline bool is_valid_multipart_boundary(const std::string& boundary)
{
    if (boundary.empty() || boundary.size() > 70 || boundary.back() == ' ')
        return false;

    for (unsigned char ch : boundary)
    {
        if (!is_multipart_boundary_char(ch))
            return false;
    }

    return true;
}

inline bool encode_multipart_quoted_value(const std::string& value,
                                          std::string *encoded)
{
    encoded->clear();
    if (value.empty())
        return false;

    for (unsigned char ch : value)
    {
        if (ch != '\t' && (ch < 0x20 || ch == 0x7f))
            return false;
    }

    encoded->reserve(value.size());
    for (char ch : value)
    {
        if (ch == '"' || ch == '\\')
            encoded->push_back('\\');
        encoded->push_back(ch);
    }

    return true;
}

} // namespace detail
} // namespace wfrest

#endif // WFREST_MULTIPARTUTIL_H_
