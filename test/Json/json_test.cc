#include "Json.h"
#include <gtest/gtest.h>

using namespace wfrest;

TEST(JsonTest, default_null)
{
    // default is null
    Json data;
    EXPECT_EQ(data.type(), JSON_VALUE_NULL);
    EXPECT_EQ(data.dump(), "null");
}

TEST(JsonTest, is_type)
{
    Json null1 = nullptr;
    EXPECT_EQ(null1.type(), JSON_VALUE_NULL);
    EXPECT_TRUE(null1.is_null());

    Json num1 = 1;
    EXPECT_EQ(num1.type(), JSON_VALUE_NUMBER);
    EXPECT_TRUE(num1.is_number());

    Json num2 = 1.0;
    EXPECT_EQ(num2.type(), JSON_VALUE_NUMBER);
    EXPECT_TRUE(num2.is_number());

    Json bool1 = true;
    EXPECT_EQ(bool1.type(), JSON_VALUE_TRUE);
    EXPECT_TRUE(bool1.is_boolean());

    Json bool2 = false;
    EXPECT_EQ(bool2.type(), JSON_VALUE_FALSE);
    EXPECT_TRUE(bool2.is_boolean());

    Json str1 = "string";
    EXPECT_EQ(str1.type(), JSON_VALUE_STRING);
    EXPECT_TRUE(str1.is_string());

    Json obj1 = Json::Object();
    EXPECT_EQ(obj1.type(), JSON_VALUE_OBJECT);
    EXPECT_TRUE(obj1.is_object());

    Json arr1 = Json::Array();
    EXPECT_EQ(arr1.type(), JSON_VALUE_ARRAY);
    EXPECT_TRUE(arr1.is_array());
}

TEST(JsonTest, const_operator)
{
    const Json js = Json::parse(
        R"({"test1":false,"test2":true,"test3":"string","test4":null})");
    EXPECT_EQ(js["test1"].get<bool>(), false);
    EXPECT_EQ(js["test3"].get<std::string>(), "string");
}

TEST(JsonTest, invalid_parse)
{
    const Json js = Json::parse(
        R"({"test1":false,"test2":true,"test3":"string","test4":null)");
    EXPECT_FALSE(js.is_valid());
}

TEST(JsonTest, create)
{
    Json data = Json::Object{
        {"null", nullptr},
        {"integer", 1},
        {"float", 1.3},
        {"boolean", true},
        {"string", "something"},
        {"array", Json::Array{1, 2}},
        {"object",
         Json::Object{
             {"key", "value"},
             {"key2", "value2"},
         }},
    };
    EXPECT_EQ(
        data.dump(),
        R"({"null":null,"integer":1,"float":1.3,"boolean":true,"string":"something","array":[1,2],"object":{"key":"value","key2":"value2"}})");
}

TEST(JsonTest, dump)
{
    Json data;
    data["key"] = 1;
    data["name"] = "chanchan";
    // using member function
    EXPECT_EQ(data.dump(), R"({"key":1,"name":"chanchan"})");
    // streams
    std::ostringstream os;
    os << data;
    EXPECT_EQ(os.str(), R"({"key":1,"name":"chanchan"})");
}

TEST(JsonTest, copy)
{
    Json data;
    data["key"] = 1;
    data["name"] = "chanchan";

    Json data1 = data.copy();
    std::cout << data1 << std::endl;
}

TEST(JsonTest, push_back_test)
{
    Json data2;
    Json data1;
    data1.push_back("key", 1);
    data2.push_back("key1", data1["key"]);
    EXPECT_EQ(data1["key"].get<int>(), 1);
    EXPECT_EQ(data2["key1"].get<int>(), 1);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
