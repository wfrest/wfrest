//
// Created by Chanchan on 10/30/21.
//

#include "StringPiece.h"
#include <string>
#include <iostream>

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

int main()
{
    test01();
}