#include <unordered_map>
#include <gtest/gtest.h>
#include "wfrest/StringPiece.h"
#include "wfrest/StrUtil.h"

using namespace wfrest;

TEST(StrUtil, ltrim)
{
    StringPiece str1("       password");
    EXPECT_EQ("password", StrUtil::ltrim(str1).as_string());
    StringPiece str2("password");
    EXPECT_EQ("password", StrUtil::ltrim(str2).as_string());
    StringPiece str3("password    ");
    EXPECT_EQ("password    ", StrUtil::ltrim(str3).as_string());
    StringPiece str4("     password    ");
    EXPECT_EQ("password    ", StrUtil::ltrim(str4).as_string());
    StringPiece str5("");
    EXPECT_EQ("", StrUtil::ltrim(str5).as_string());
    StringPiece str6("      ");
    EXPECT_EQ("", StrUtil::ltrim(str6).as_string());
    StringPiece str7("  name password   ");
    EXPECT_EQ("name password   ", StrUtil::ltrim(str7).as_string());
}

TEST(StrUtil, rtrim)
{
    StringPiece str1("name        ");
    EXPECT_EQ("name", StrUtil::rtrim(str1).as_string());
    StringPiece str2("name");
    EXPECT_EQ("name", StrUtil::rtrim(str2).as_string());
    StringPiece str3("      name");
    EXPECT_EQ("      name", StrUtil::rtrim(str3).as_string());
    StringPiece str4("      name     ");
    EXPECT_EQ("      name", StrUtil::rtrim(str4).as_string());
    StringPiece str5("");
    EXPECT_EQ("", StrUtil::rtrim(str5).as_string());
    StringPiece str6("      ");
    EXPECT_EQ("", StrUtil::rtrim(str6).as_string());
    StringPiece str7("  name password   ");
    EXPECT_EQ("  name password", StrUtil::rtrim(str7).as_string());
}

TEST(StrUtil, trim)
{
    StringPiece str1("    name        ");
    EXPECT_EQ("name", StrUtil::trim(str1).as_string());
    StringPiece str2("name");
    EXPECT_EQ("name", StrUtil::trim(str2).as_string());
    StringPiece str3("      name");
    EXPECT_EQ("name", StrUtil::trim(str3).as_string());
    StringPiece str4("name     ");
    EXPECT_EQ("name", StrUtil::trim(str4).as_string());
    StringPiece str5("");
    EXPECT_EQ("", StrUtil::trim(str5).as_string());
    StringPiece str6("      ");
    EXPECT_EQ("", StrUtil::trim(str6).as_string());
    StringPiece str7("  name password   ");
    EXPECT_EQ("name password", StrUtil::trim(str7).as_string());
    StringPiece str8(" \"name  name  address password\"");
    EXPECT_EQ("\"name  name  address password\"", StrUtil::trim(str8).as_string());
}

TEST(StrUtil, trim_pairs)
{
    StringPiece str1("\"name  name  address password\"");
    EXPECT_EQ("name  name  address password", StrUtil::trim_pairs(str1, R"(""'')").as_string());
    StringPiece str2("[name {} name  address password]");
    EXPECT_EQ("name {} name  address password", StrUtil::trim_pairs(str2).as_string());
}

TEST(StrUtil, split_piece)
{
    StringPiece str("host=admin&password=123&time=10000101");

    std::vector<std::string> str_list = StrUtil::split_piece<std::string>(str, '&');
    EXPECT_EQ("host=admin", str_list[0]);
    EXPECT_EQ("password=123", str_list[1]);
    EXPECT_EQ("time=10000101", str_list[2]);
}

TEST(StringCaseLess, map)
{
    std::map<std::string, std::string, MapStringCaseLess> map;
    map["Content-Encoding"] = "gzip";
    map["content-encoding"] = "br";
    EXPECT_TRUE(map.size() == 1);
    EXPECT_TRUE(map.count("conteNt-eNcodIng"));
}
