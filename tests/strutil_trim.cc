#include "StrUtil.h"

using namespace wfrest;


void test01()
{
    StringPiece str1("       aas");
    StringPiece trim1 = StrUtil::ltrim(str1);
    fprintf(stderr, "%s\n", trim1.as_string().c_str());

    StringPiece str2("aabasd        ");
    StringPiece trim2 = StrUtil::rtrim(str2);
    fprintf(stderr, "%s\n", trim2.as_string().c_str());

    StringPiece str3("    aabasd        ");
    StringPiece trim3 = StrUtil::trim(str3);
    fprintf(stderr, "%s\n", trim3.as_string().c_str());

    StringPiece str4("");
    StringPiece trim4 = StrUtil::trim(str4);
    fprintf(stderr, "%s\n", trim4.as_string().c_str());

    StringPiece str5(" \"asda  asda  agbag asd\"");
    StringPiece trim5 = StrUtil::trim(str5);
    fprintf(stderr, "%s\n", trim5.as_string().c_str());
    fprintf(stderr, "%zu -> %zu\n", str5.size(), trim5.size());
    StringPiece tp = StrUtil::trim_pairs(trim5, R"(""'')");
    fprintf(stderr, "%s\n", tp.as_string().c_str());
}

int main()
{
    test01();
}
