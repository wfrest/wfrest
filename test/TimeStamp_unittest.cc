#include <gtest/gtest.h>
#include "wfrest/Timestamp.h"
#include <iostream>

using namespace wfrest;

int main()
{
    Timestamp ts(1639279032782231L);
    std::cout << ts.to_format_str() << std::endl;
    std::cout << ts.to_format_str("%a, %d %b %Y %H:%M:%S GMT") << std::endl;

    return 0;
}
