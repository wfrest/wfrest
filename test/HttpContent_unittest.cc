#include <gtest/gtest.h>
#include "wfrest/HttpContent.h"
#include "wfrest/StringPiece.h"
#include "wfrest/UriUtil.h"

using namespace wfrest;

TEST(Urlencode, parse_post_kv)
{
    const std::string payload =
        "token=a=b=c&message=hello+world&expr=a%26b%3Dc&"
        "flag&empty=&%61=first&a=second&bad=%ZZ&nul=%00";

    const auto form = Urlencode::parse_post_kv(StringPiece(payload));
    const auto query = UriUtil::split_query(StringPiece(payload));

    EXPECT_EQ(form, query);
    ASSERT_EQ(form.size(), 8U);
    EXPECT_EQ(form.at("token"), "a=b=c");
    EXPECT_EQ(form.at("message"), "hello world");
    EXPECT_EQ(form.at("expr"), "a&b=c");
    EXPECT_EQ(form.at("flag"), "");
    EXPECT_EQ(form.at("empty"), "");
    EXPECT_EQ(form.at("a"), "first");
    EXPECT_EQ(form.at("bad"), "%ZZ");
    ASSERT_EQ(form.at("nul").size(), 1U);
    EXPECT_EQ(form.at("nul")[0], '\0');

}
