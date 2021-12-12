#include <unordered_map>
#include <gtest/gtest.h>
#include "wfrest/HttpCookie.h"

using namespace wfrest;

TEST(HttpCookie, dump)
{
    HttpCookie cookie("user", "wfrest");
    EXPECT_TRUE(cookie);

    EXPECT_EQ(cookie.dump(), "Set-Cookie: user=wfrest\r\n");

    cookie.set_secure(true);
    cookie.set_path("/");

    EXPECT_EQ(cookie.dump(), "Set-Cookie: user=wfrest; Path=/; Secure\r\n");

    cookie.set_expires(Timestamp(1639279032782231L));

    EXPECT_EQ(cookie.dump(), "Set-Cookie: user=wfrest; Expires=Sun, 12 Dec 2021 11:17:12 GMT; Path=/; Secure\r\n");
    
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}