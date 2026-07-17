#include "HttpCookie.h"

using namespace wfrest;

namespace
{

bool is_token_char(unsigned char ch)
{
    if ((ch >= '0' && ch <= '9') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= 'a' && ch <= 'z'))
    {
        return true;
    }

    switch (ch)
    {
        case '!': case '#': case '$': case '%': case '&': case '\'':
        case '*': case '+': case '-': case '.': case '^': case '_':
        case '`': case '|': case '~':
            return true;
        default:
            return false;
    }
}

bool is_cookie_octet(unsigned char ch)
{
    return ch == 0x21 ||
           (ch >= 0x23 && ch <= 0x2B) ||
           (ch >= 0x2D && ch <= 0x3A) ||
           (ch >= 0x3C && ch <= 0x5B) ||
           (ch >= 0x5D && ch <= 0x7E);
}

bool is_valid_name(const std::string &name)
{
    if (name.empty())
        return false;

    for (unsigned char ch : name)
    {
        if (!is_token_char(ch))
            return false;
    }
    return true;
}

bool is_valid_value(const std::string &value)
{
    for (unsigned char ch : value)
    {
        if (!is_cookie_octet(ch))
            return false;
    }
    return true;
}

bool is_valid_attribute(const std::string &value)
{
    for (unsigned char ch : value)
    {
        if (ch < 0x20 || ch > 0x7E || ch == ';')
            return false;
    }
    return true;
}

void trim_ows(const char **begin, const char **end)
{
    while (*begin < *end && (**begin == ' ' || **begin == '\t'))
        ++*begin;
    while (*end > *begin && ((*end)[-1] == ' ' || (*end)[-1] == '\t'))
        --*end;
}

bool parse_value(const char *begin, const char *end, std::string *value)
{
    if (begin < end && (*begin == '"' || end[-1] == '"'))
    {
        if (end - begin < 2 || *begin != '"' || end[-1] != '"')
            return false;
        ++begin;
        --end;
    }

    value->assign(begin, static_cast<size_t>(end - begin));
    return is_valid_value(*value);
}

} // namespace

std::map<std::string, std::string> HttpCookie::split(
    const StringPiece &cookie_piece)
{
    std::map<std::string, std::string> result;
    if (cookie_piece.empty())
        return result;

    const char *segment = cookie_piece.begin();

    while (segment < cookie_piece.end())
    {
        const char *segment_end = segment;
        while (segment_end < cookie_piece.end() && *segment_end != ';')
            ++segment_end;

        const char *separator = segment;
        while (separator < segment_end && *separator != '=')
            ++separator;

        if (separator < segment_end)
        {
            const char *name_begin = segment;
            const char *name_end = separator;
            const char *value_begin = separator + 1;
            const char *value_end = segment_end;
            trim_ows(&name_begin, &name_end);
            trim_ows(&value_begin, &value_end);

            std::string name(name_begin, static_cast<size_t>(name_end - name_begin));
            std::string value;
            if (is_valid_name(name) && parse_value(value_begin, value_end, &value))
                result.emplace(std::move(name), std::move(value));
        }

        segment = segment_end < cookie_piece.end()
                      ? segment_end + 1
                      : cookie_piece.end();
    }

    return result;
}

HttpCookie::operator bool() const
{
    return is_valid_name(key_) && is_valid_value(value_) &&
           is_valid_attribute(domain_) && is_valid_attribute(path_);
}

std::string HttpCookie::dump() const
{
    if (!static_cast<bool>(*this))
        return {};

    std::string ret;
    ret.reserve(key_.size() + value_.size() + 30);
    ret.append(key_).append("=").append(value_).append("; ");
    if (has_max_age_)
    {
        ret.append("Max-Age=").append(std::to_string(max_age_)).append("; ");
    }
    else if (expires_.valid())
    {
        ret.append("Expires=")
                .append(expires_.to_utc_format_str(
                    "%a, %d %b %Y %H:%M:%S GMT"))
                .append("; ");
    }
    if (!domain_.empty())
        ret.append("Domain=").append(domain_).append("; ");
    if (!path_.empty())
        ret.append("Path=").append(path_).append("; ");
    if (secure_)
        ret.append("Secure; ");
    if (http_only_)
        ret.append("HttpOnly; ");

    const std::string same_site = same_site_to_str(same_site_);
    if (!same_site.empty())
        ret.append("SameSite=").append(same_site).append("; ");
    if (same_site_ == SameSite::NONE && !secure_)
        ret.append("Secure; ");

    ret.resize(ret.size() - 2);
    return ret;
}
