#ifndef WFREST_HTTPHEADERUTIL_H_
#define WFREST_HTTPHEADERUTIL_H_

#include <cstddef>
#include <string>

namespace wfrest
{
namespace detail
{

inline unsigned char header_ascii_lower(unsigned char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return static_cast<unsigned char>(ch + ('a' - 'A'));
    return ch;
}

inline bool header_ascii_iequals(const std::string &value,
                                 const char *expected)
{
    size_t cursor = 0;
    while (cursor < value.size() && expected[cursor] != '\0')
    {
        if (header_ascii_lower(static_cast<unsigned char>(value[cursor])) !=
            header_ascii_lower(static_cast<unsigned char>(expected[cursor])))
        {
            return false;
        }
        ++cursor;
    }
    return cursor == value.size() && expected[cursor] == '\0';
}

inline bool is_header_token_char(unsigned char ch)
{
    if ((ch >= '0' && ch <= '9') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= 'a' && ch <= 'z'))
    {
        return true;
    }

    switch (ch)
    {
        case '!':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '*':
        case '+':
        case '-':
        case '.':
        case '^':
        case '_':
        case '`':
        case '|':
        case '~':
            return true;
        default:
            return false;
    }
}

inline bool is_valid_response_header_name(const std::string &name)
{
    if (name.empty())
        return false;
    for (unsigned char ch : name)
    {
        if (!is_header_token_char(ch))
            return false;
    }
    return true;
}

inline bool is_valid_response_header_value(const std::string &value)
{
    for (unsigned char ch : value)
    {
        if (ch != '\t' && (ch < ' ' || ch == 0x7f))
            return false;
    }
    return true;
}

inline bool is_framework_managed_response_header(const std::string &name)
{
    return header_ascii_iequals(name, "Content-Length") ||
           header_ascii_iequals(name, "Transfer-Encoding");
}

inline bool is_application_response_header(const std::string &name,
                                           const std::string &value)
{
    return is_valid_response_header_name(name) &&
           is_valid_response_header_value(value) &&
           !is_framework_managed_response_header(name);
}

template<typename HeaderMap>
void sanitize_application_response_headers(HeaderMap *headers)
{
    auto current = headers->begin();
    while (current != headers->end())
    {
        if (is_application_response_header(current->first, current->second))
        {
            ++current;
        }
        else
        {
            const auto invalid = current++;
            headers->erase(invalid);
        }
    }
}

inline bool is_ows_byte(char ch)
{
    return ch == ' ' || ch == '\t';
}

inline bool is_token_range(const std::string &value,
                           size_t begin,
                           size_t end)
{
    if (begin == end)
        return false;
    for (size_t cursor = begin; cursor < end; ++cursor)
    {
        if (!is_header_token_char(
                static_cast<unsigned char>(value[cursor])))
        {
            return false;
        }
    }
    return true;
}

inline bool header_ascii_range_iequals(const std::string &value,
                                       size_t begin,
                                       size_t end,
                                       const char *expected)
{
    size_t cursor = begin;
    size_t expected_cursor = 0;
    while (cursor < end && expected[expected_cursor] != '\0')
    {
        if (header_ascii_lower(static_cast<unsigned char>(value[cursor])) !=
            header_ascii_lower(
                static_cast<unsigned char>(expected[expected_cursor])))
        {
            return false;
        }
        ++cursor;
        ++expected_cursor;
    }
    return cursor == end && expected[expected_cursor] == '\0';
}

inline bool is_json_response_content_type(const std::string &value)
{
    if (!is_valid_response_header_value(value))
        return false;

    const size_t semicolon = value.find(';');
    size_t begin = 0;
    size_t end = semicolon == std::string::npos
                     ? value.size()
                     : semicolon;
    while (begin < end && is_ows_byte(value[begin]))
        ++begin;
    while (end > begin && is_ows_byte(value[end - 1]))
        --end;

    const size_t slash = value.find('/', begin);
    if (slash == std::string::npos || slash >= end ||
        value.find('/', slash + 1) < end ||
        !is_token_range(value, begin, slash) ||
        !is_token_range(value, slash + 1, end))
    {
        return false;
    }

    const size_t subtype_begin = slash + 1;
    if (header_ascii_range_iequals(value, subtype_begin, end, "json"))
        return true;

    const size_t suffix_size = 5;
    return end - subtype_begin >= suffix_size &&
           header_ascii_range_iequals(value, end - suffix_size, end,
                                      "+json");
}

} // namespace detail
} // namespace wfrest

#endif // WFREST_HTTPHEADERUTIL_H_
