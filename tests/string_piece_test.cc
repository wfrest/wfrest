//
// Created by Chanchan on 10/30/21.
//

#include "StringPiece.h"
#include <string>
#include <iostream>
#include <unordered_map>

using namespace wfrest;

void test01()
{
    std::string str = "0123456";
    int cursor = 1;
    StringPiece strp1(&str[cursor], 4);
    std::cout << strp1.as_string() << std::endl;

    StringPiece strp2(strp1.data() + cursor, 2);
    std::cout << strp2.as_string() << std::endl;
}

void test02()
{
    std::unordered_map<StringPiece, int, StringPieceHash> piece_map;
    {
        std::string a = "12345";
        StringPiece b(a.c_str() + 1, 3);
        std::cout << b.as_string() << std::endl;
        piece_map[b] = 1;
    }
    StringPiece c("234");
    auto it = piece_map.find(c);
    if(it != piece_map.end())
    {
        std::cout << "find it" << std::endl;
    }
    else
    {
        std::cout << "not found" << std::endl;
    }
}

void proc_param(StringPiece& str)
{
    int i = 1;
    int j = str.size() - 2;
    while(str[i] == ' ') i++;
    while(str[j] == ' ') j--;
    str.shrink(i, str.size() - 1 - j);
}
void test03()
{
    StringPiece str1("<    name   >");
    proc_param(str1);
    std::cout << str1.as_string() << std::endl;

    StringPiece str2("<name>");
    proc_param(str2);
    std::cout << str2.as_string() << std::endl;

    StringPiece str3("<>");
    proc_param(str3);
    std::cout << str3.as_string() << std::endl;
}
int main()
{
    test03();
}