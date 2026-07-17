#include <gtest/gtest.h>

#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#include "wfrest/HttpCookie.h"

using namespace wfrest;

namespace
{

class ScopedTimezone
{
public:
    explicit ScopedTimezone(const char *timezone)
    {
        const char *current = std::getenv("TZ");
        if (current != nullptr)
        {
            had_value_ = true;
            value_ = current;
        }

        (void)setenv("TZ", timezone, 1);
        tzset();
    }

    ~ScopedTimezone()
    {
        if (had_value_)
            (void)setenv("TZ", value_.c_str(), 1);
        else
            (void)unsetenv("TZ");
        tzset();
    }

private:
    bool had_value_ = false;
    std::string value_;
};

} // namespace

TEST(HttpCookie, dump)
{
    HttpCookie cookie("user", "wfrest");
    EXPECT_TRUE(cookie);

    EXPECT_EQ(cookie.dump(), "user=wfrest");

    cookie.set_secure(true).set_path("/");

    EXPECT_EQ(cookie.dump(), "user=wfrest; Path=/; Secure");
}

TEST(HttpCookie, same_site)
{
    HttpCookie cookie("user", "wfrest");
    cookie.set_domain("/")
            .set_max_age(1000)
            .set_same_site(SameSite::NONE);

    EXPECT_EQ(cookie.dump(), "user=wfrest; Max-Age=1000; Domain=/; SameSite=None; Secure");
}

TEST(HttpCookie, expires_uses_utc_when_process_timezone_does_not)
{
    ScopedTimezone timezone("EST5");
    HttpCookie cookie("session", "value");
    cookie.set_expires(Timestamp(1000000));

    EXPECT_EQ(cookie.dump(),
              "session=value; Expires=Thu, 01 Jan 1970 00:00:01 GMT");
}

TEST(HttpCookie, split)
{
    const auto res = HttpCookie::split(StringPiece(
        "user=chanchan; passwd=123; token=a=b=c; flag=; quoted=\"abc\""));
    ASSERT_EQ(res.size(), 5U);
    EXPECT_EQ(res.at("user"), "chanchan");
    EXPECT_EQ(res.at("passwd"), "123");
    EXPECT_EQ(res.at("token"), "a=b=c");
    EXPECT_EQ(res.at("flag"), "");
    EXPECT_EQ(res.at("quoted"), "abc");
}

TEST(HttpCookie, split_trim)
{
    StringPiece cookie("  user  =  chanchan ;  passwd = 123    ");
    std::map<std::string, std::string> res = HttpCookie::split(cookie);
    auto it = res.begin();
    EXPECT_EQ("passwd", it->first);
    EXPECT_EQ("123", it->second);
    it++;
    EXPECT_EQ("user", it->first);
    EXPECT_EQ("chanchan", it->second);
}

TEST(HttpCookie, split_skips_malformed_and_keeps_first)
{
    const auto res = HttpCookie::split(StringPiece(
        "missing; =value; bad name=x; bad=has space; broken=\"quote; "
        "a=first; a=second; comma=one,two; valid=yes"));
    ASSERT_EQ(res.size(), 2U);
    EXPECT_EQ(res.at("a"), "first");
    EXPECT_EQ(res.at("valid"), "yes");
}

TEST(HttpCookie, empty_value_and_signed_max_age)
{
    HttpCookie cookie("session", "");
    EXPECT_TRUE(cookie);
    cookie.set_path("/").set_http_only(true).set_max_age(0);
    EXPECT_TRUE(cookie.has_max_age());
    EXPECT_EQ(cookie.dump(), "session=; Max-Age=0; Path=/; HttpOnly");

    cookie.set_max_age(-1);
    EXPECT_EQ(cookie.dump(), "session=; Max-Age=-1; Path=/; HttpOnly");
}

TEST(HttpCookie, rejects_unsafe_output)
{
    const std::vector<std::string> unsafe_values = {
        "space value", "quote\"", "comma,", "semi;", "slash\\",
        "line\rbreak", "line\nbreak", std::string(1, '\x7f'),
        std::string(1, static_cast<char>(0x80))};
    for (const std::string &value : unsafe_values)
    {
        HttpCookie cookie("name", value);
        EXPECT_FALSE(cookie);
        EXPECT_TRUE(cookie.dump().empty());
    }

    EXPECT_TRUE(HttpCookie("bad name", "value").dump().empty());

    HttpCookie domain("name", "value");
    domain.set_domain("example.com\r\nInjected: yes");
    EXPECT_TRUE(domain.dump().empty());

    HttpCookie path("name", "value");
    path.set_path("/; injected=yes");
    EXPECT_TRUE(path.dump().empty());
}
