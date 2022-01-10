#include <gtest/gtest.h>
#include "wfrest/Timestamp.h"
#include <iostream>

using namespace wfrest;

TEST(Timestamp, format_str)
{
    Timestamp ts(1639279032782231L);
    
    EXPECT_EQ("2021-12-12 11:17:12", ts.to_format_str());

    EXPECT_EQ("Sun, 12 Dec 2021 11:17:12 GMT", ts.to_format_str("%a, %d %b %Y %H:%M:%S GMT"));
}

int main(int argc, char **argv) 
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


