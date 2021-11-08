//
// Created by Chanchan on 11/7/21.
//

#include "StrUtil.h"

using namespace wfrest;

void test01()
{
    StringPiece str("host=admin&password=123&time=10000101");
    auto str_list = StrUtil::split_piece<std::string>(str, '&');
    for(auto &s : str_list)
    {
        fprintf(stderr, "%s\n", s.c_str());
    }
}

void test02()
{
    StringPiece str("host=admin&password=123&time=10000101");
    auto str_list = StrUtil::split_piece<StringPiece>(str, '&');
    for(auto &s : str_list)
    {
        fprintf(stderr, "%s\n", s.data());
    }
}

void test03()
{
    StringPiece str1("       aas");
    StringPiece a = StrUtil::ltrim(str1);
    fprintf(stderr, "%s\n", a.data());
}

void test04()
{
    StringPiece str2("aabasd        ");
    StringPiece b = StrUtil::rtrim(str2);
    fprintf(stderr, "%s\n", b.data());
}

void test05()
{
    StringPiece str3("    aabasd        ");
    StringPiece c = StrUtil::trim(str3);
    fprintf(stderr, "%s\n", c.data());
}

void test06()
{
    StringPiece str4("");
    StringPiece d = StrUtil::trim(str4);
    fprintf(stderr, "%s\n", d.data());
}

void test07()
{
    StringPiece str5(" asda  asda  agbag asd     ");
    StringPiece e = StrUtil::trim(str5);
    fprintf(stderr, "%s\n", e.data());
    fprintf(stderr, "%zu -> %zu", str5.size(), e.size());
}


int main()
{
//    test01();
//    fprintf(stderr, "\n");
//    test02();
    test03();
    test04();
    test05();
    test06();
    test07();

}
