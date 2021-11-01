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

int main()
{
    test02();
}