#ifndef WFREST_HTTPCOOKIE_H_
#define WFREST_HTTPCOOKIE_H_

#include <string>
#include <map>
#include "Timestamp.h"
#include "StringPiece.h"
#include "Copyable.h"
namespace wfrest
{

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Cookie
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Cookies

enum class SameSite
{
    DEFAULT, STRICT, LAX, NONE
};

inline const char *same_site_to_str(SameSite same_site)
{
    switch (same_site)
    {
        case SameSite::STRICT:
            return "Strict";
        case SameSite::LAX:
            return "Lax";
        case SameSite::NONE:
            return "None";
        default:
            return "";
    }
}

class HttpCookie : public Copyable
{
public:
    // Check if the cookie is empty
    explicit operator bool() const
    { return (!key_.empty()) && (!value_.empty()); }

    std::string dump() const;

    static std::map<std::string, std::string> split(const StringPiece &cookie_piece);

public:
    HttpCookie &set_key(const std::string &key)
    {
        key_ = key;
        return *this;
    }

    HttpCookie &set_key(std::string &&key)
    {
        key_ = std::move(key);
        return *this;
    }

    HttpCookie &set_value(const std::string &value)
    {
        value_ = value;
        return *this;
    }

    HttpCookie &set_value(std::string &&value)
    {
        value_ = std::move(value);
        return *this;
    }

    HttpCookie &set_domain(const std::string &domain)
    {
        domain_ = domain;
        return *this;
    }

    HttpCookie &set_domain(std::string &&domain)
    {
        domain_ = std::move(domain);
        return *this;
    }

    HttpCookie &set_path(const std::string &path)
    {
        path_ = path;
        return *this;
    }

    HttpCookie &set_path(std::string &&path)
    {
        path_ = std::move(path);
        return *this;
    }

    HttpCookie &set_expires(const Timestamp &expires)
    {
        expires_ = expires;
        return *this;
    }

    HttpCookie &set_max_age(int max_age)
    {
        max_age_ = max_age;
        return *this;
    }

    HttpCookie &set_secure(bool secure)
    {
        secure_ = secure;
        return *this;
    }

    HttpCookie &set_http_only(bool http_only)
    {
        http_only_ = http_only;
        return *this;
    }

    HttpCookie &set_same_site(SameSite same_site)
    {
        same_site_ = same_site;
        return *this;
    }

public:
    const std::string &key() const
    { return key_; }

    const std::string &value() const
    { return value_; }

    const std::string &domain() const
    { return domain_; }

    const std::string &path() const
    { return path_; }

    Timestamp expires() const
    { return expires_; }

    int max_age() const
    { return max_age_; }

    bool is_secure() const
    { return secure_; }

    bool is_http_only() const
    { return http_only_; }

    SameSite same_site() const
    { return same_site_; }

public:
    HttpCookie(const std::string &key, const std::string &value)
            : key_(key), value_(value)
    {}

    HttpCookie(std::string &&key, std::string &&value)
            : key_(std::move(key)), value_(std::move(value))
    {}

    HttpCookie() = default;

private:
    std::string key_;
    std::string value_;
    std::string domain_;
    std::string path_;

    Timestamp expires_;
    int max_age_ = 0;
    bool secure_ = false;
    bool http_only_ = false;

    SameSite same_site_ = SameSite::DEFAULT;
};

} // namespace wfrest




#endif // WFREST_HTTPCOOKIE_H_