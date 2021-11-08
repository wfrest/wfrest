//
// Created by Chanchan on 11/8/21.
//


#include "StrUtil.h"

using namespace wfrest;


void test01()
{
    StringPiece str1("       aas");
    StringPiece trim1 = StrUtil::ltrim(str1);
    fprintf(stderr, "%s\n", trim1.data());

    StringPiece str2("aabasd        ");
    StringPiece trim2 = StrUtil::rtrim(str2);
    fprintf(stderr, "%s\n", trim2.data());

    StringPiece str3("    aabasd        ");
    StringPiece trim3 = StrUtil::trim(str3);
    fprintf(stderr, "%s\n", trim3.data());

    StringPiece str4("");
    StringPiece trim4 = StrUtil::trim(str4);
    fprintf(stderr, "%s\n", trim4.data());

    StringPiece str5(" \"asda  asda  agbag asd\"");
    StringPiece trim5 = StrUtil::trim(str5);
    fprintf(stderr, "%s\n", trim5.data());
    fprintf(stderr, "%zu -> %zu", str5.size(), trim5.size());
}

int main()
{
    test01();
}
