#include <gtest/gtest.h>
#include "wfrest/base64.h"

using namespace wfrest;

TEST(base64, shortText)
{
    std::string source = "wfrest http framework";

    std::string encode = Base64::encode(
                        reinterpret_cast<const unsigned char *>(source.data()),
                        source.size());
    EXPECT_EQ(encode, "d2ZyZXN0IGh0dHAgZnJhbWV3b3Jr");
    std::string decode = Base64::decode(encode);
    EXPECT_EQ(source, decode);
}

TEST(base64, longText)
{
    std::string source;
    source.reserve(100000);
    for (int i = 0; i < 100000; ++i)
    {
        source.append(1, char(i));
    }

    std::string encode = Base64::encode(
                        reinterpret_cast<const unsigned char *>(source.data()),
                        source.size());
    std::string decode = Base64::decode(encode);
    EXPECT_EQ(source, decode);
}
