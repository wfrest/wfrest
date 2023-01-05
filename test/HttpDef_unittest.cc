#include <unordered_map>
#include <gtest/gtest.h>
#include "wfrest/HttpDef.h"

using namespace wfrest;

TEST(ContentType, to_str)
{
    http_content_type content_type = APPLICATION_OCTET_STREAM;
    EXPECT_EQ(ContentType::to_str(content_type), "application/octet-stream");
}

TEST(ContentType, to_str_by_suffix)
{
    EXPECT_EQ(ContentType::to_str_by_suffix("txt"), "text/plain");
}

TEST(ContentType, to_enum)
{
    std::string type_str = "application/javascript";
    EXPECT_EQ(ContentType::to_enum(type_str), APPLICATION_JAVASCRIPT);
}

TEST(ContentType, to_enum_by_suffix)
{
    EXPECT_EQ(ContentType::to_enum_by_suffix("mp"), MULTIPART_FORM_DATA);
}
