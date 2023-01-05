#include <unordered_map>
#include <gtest/gtest.h>
#include "wfrest/HttpCookie.h"

using namespace wfrest;

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

TEST(HttpCookie, split)
{
    StringPiece cookie("user=chanchan,passwd=123");
    std::map<std::string, std::string> res = HttpCookie::split(cookie);
    auto it = res.begin();
    EXPECT_EQ("passwd", it->first);
    EXPECT_EQ("123", it->second);
    it++;
    EXPECT_EQ("user", it->first);
    EXPECT_EQ("chanchan", it->second);
}

TEST(HttpCookie, split_trim)
{
    StringPiece cookie("  user  =  chanchan ,  passwd = 123    ");
    std::map<std::string, std::string> res = HttpCookie::split(cookie);
    auto it = res.begin();
    EXPECT_EQ("passwd", it->first);
    EXPECT_EQ("123", it->second);
    it++;
    EXPECT_EQ("user", it->first);
    EXPECT_EQ("chanchan", it->second);
}
