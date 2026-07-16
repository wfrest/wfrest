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
    EXPECT_EQ(ContentType::to_enum("application/javascript"),
              APPLICATION_JAVASCRIPT);
    EXPECT_EQ(ContentType::to_enum(" Application/JSON\t; charset=utf-8"),
              APPLICATION_JSON);
    EXPECT_EQ(ContentType::to_enum("Multipart/Form-Data; Boundary=abc"),
              MULTIPART_FORM_DATA);
}

TEST(ContentType, to_enum_rejects_prefixes_and_empty_tokens)
{
    EXPECT_EQ(ContentType::to_enum(""), CONTENT_TYPE_NONE);
    EXPECT_EQ(ContentType::to_enum(" \t"), CONTENT_TYPE_NONE);
    EXPECT_EQ(ContentType::to_enum("application/jsonp"),
              CONTENT_TYPE_UNDEFINED);
    EXPECT_EQ(ContentType::to_enum("multipart/form-datax; boundary=x"),
              CONTENT_TYPE_UNDEFINED);
    EXPECT_EQ(ContentType::to_enum("; charset=utf-8"),
              CONTENT_TYPE_UNDEFINED);
}

TEST(ContentType, to_enum_by_suffix)
{
    EXPECT_EQ(ContentType::to_enum_by_suffix("mp"), MULTIPART_FORM_DATA);
}
