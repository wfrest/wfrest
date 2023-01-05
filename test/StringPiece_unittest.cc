#include <gtest/gtest.h>
#include <unordered_map>
#include "wfrest/StringPiece.h"

using namespace wfrest;

TEST(StringPiece, constructor)
{
    std::string str = "0123456";
    int cursor = 1;
    StringPiece strp1(&str[cursor], 4);
    EXPECT_EQ("1234", strp1.as_string());

    StringPiece strp2(strp1.data() + cursor, 2);
    EXPECT_EQ("23", strp2.as_string());
}

TEST(StringPiece, map)
{
    std::map<StringPiece, int> piece_map;
    std::string a = "12345";
    StringPiece b(a.c_str() + 1, 3);

    EXPECT_EQ("234", b.as_string());
    piece_map[b] = 1;   // "234" : 1

    StringPiece c("234");
    auto it = piece_map.find(c);
    EXPECT_TRUE(it != piece_map.end());
    EXPECT_EQ(piece_map[c], 1);

    char *addr = &*a.begin()+1;
    EXPECT_EQ(addr, b.data());
    EXPECT_EQ(addr, it->first.data());
}

void proc_param(StringPiece& str)
{
    int i = 1;
    int j = str.size() - 2;
    while(str[i] == ' ') i++;
    while(str[j] == ' ') j--;
    str.shrink(i, str.size() - 1 - j);
}

TEST(StringPiece, get_parameter)
{
    StringPiece str1("<    name   >");
    proc_param(str1);
    EXPECT_EQ("name", str1.as_string());

    StringPiece str2("<name>");
    proc_param(str2);
    EXPECT_EQ("name", str2.as_string());

    StringPiece str3("<>");
    proc_param(str3);
    EXPECT_EQ("", str3.as_string());
}

TEST(StringPiece, wildcast)
{
    StringPiece origin("action*");
    StringPiece match(origin);
    match.remove_suffix(1);
    StringPiece str1("action1");
    StringPiece str2("action");
    StringPiece str3("action123123");
    EXPECT_TRUE(str1.starts_with(match));
    EXPECT_TRUE(str2.starts_with(match));
    EXPECT_TRUE(str3.starts_with(match));
}

TEST(StringPiece, shrink)
{
    StringPiece str("1234567890");
    str.shrink(0, 2);
    EXPECT_EQ("12345678", str.as_string());
}
