#include "HttpCookie.h"

using namespace wfrest;

std::string HttpCookie::dump() const
{
    std::string ret = "Set-Cookie: ";
    ret.reserve(ret.size() + key_.size() + value_.size() + 30);
    ret.append(key_).append("=").append(value_).append("; ");
    // If both Expires and Max-Age are set, Max-Age has precedence.
    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie
    bool has_max_age = max_age_ > 0;
    if (has_max_age) 
    {
        ret.append("Max-Age").append("=").append(std::to_string(max_age_)).append("; ");
    }
    if(!has_max_age && expires_.valid())
    {
        ret.append("Expires=")
            .append(expires_.to_format_str("%a, %d %b %Y %H:%M:%S GMT"))
            .append("; ");
    }

    if (!domain_.empty())
    {
        ret.append("Domain=").append(domain_).append("; ");
    }
    if (!path_.empty())
    {
        ret.append("Path=").append(path_).append("; ");
    }
    if (secure_)
    {
        ret.append("Secure; ");
    }
    if (http_only_)
    {
        ret.append("HttpOnly; ");
    }
    ret.resize(ret.length() - 2);  // delete last semicolon
    ret.append("\r\n");
    return ret;
}
