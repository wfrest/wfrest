#include <vector>
#include "HttpCookie.h"
#include "StrUtil.h"

using namespace wfrest;

std::map<std::string, std::string> HttpCookie::split(const StringPiece &cookie_piece)
{
    std::map<std::string, std::string> res;

    if (cookie_piece.empty())
        return res;

    // user=wfrest, passwd=123
    // [user=wfest] [ passwd=123]
    std::vector<StringPiece> arr = StrUtil::split_piece<StringPiece>(cookie_piece, ',');

    if (arr.empty())
        return res;

    std::map<StringPiece, StringPiece> piece_res;

    for (const auto &ele: arr)
    {
        if (ele.empty())
            continue;

        // [user, wfrest]
        std::vector<StringPiece> kv = StrUtil::split_piece<StringPiece>(ele, '=');
        size_t kv_size = kv.size();
        StringPiece &key = kv[0];

        if (key.empty() || piece_res.count(key) > 0)
            continue;

        if (kv_size == 1)
        {
            piece_res.emplace(StrUtil::trim(key), "");
            continue;
        }

        StringPiece &val = kv[1];

        if (val.empty())
            piece_res.emplace(StrUtil::trim(key), "");
        else
            piece_res.emplace(StrUtil::trim(key), StrUtil::trim(val));
    }
    for (auto &piece: piece_res)
    {
        res.emplace(piece.first.as_string(), piece.second.as_string());
    }

    return res;
}

std::string HttpCookie::dump() const
{
    std::string ret;
    ret.reserve(key_.size() + value_.size() + 30);
    ret.append(key_).append("=").append(value_).append("; ");
    // If both Expires and Max-Age are set, Max-Age has precedence.
    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie
    bool has_max_age = max_age_ > 0;
    if (has_max_age)
    {
        ret.append("Max-Age").append("=").append(std::to_string(max_age_)).append("; ");
    }
    if (!has_max_age && expires_.valid())
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
    std::string same_site = same_site_to_str(same_site_);
    if(!same_site.empty())
    {
        ret.append("SameSite=").append(same_site).append("; ");
    }
    // Cookies with SameSite=None must now also specify 
    // the Secure attribute (they require a secure context/HTTPS).
    if(same_site == "None" && !secure_)
    {
        ret.append("Secure; ");
    } 
    ret.resize(ret.length() - 2);  
    return ret;
}
