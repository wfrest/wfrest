#include "Json.h"
#include <gtest/gtest.h>

using namespace wfrest;

TEST(ObjTest, create_obj)
{
    Json data = Json::Object{{"key1", 123}, {"key2", true}};
    EXPECT_EQ(data.dump(), R"({"key1":123,"key2":true})");
}

TEST(ObjTest, empty_obj)
{
    Json data = Json::Object();
    EXPECT_TRUE(data.is_object());
    EXPECT_EQ(data.dump(), "{}");
}

TEST(ObjTest, one_level)
{
    Json data;
    EXPECT_TRUE(data.is_null());
    Json data_tmp = data["test"];
    EXPECT_TRUE(data_tmp.is_null());
    data["test"] = 1.0;
    EXPECT_EQ(data.type(), JSON_VALUE_OBJECT);
    EXPECT_TRUE(data.is_object());
    EXPECT_EQ(data.dump(), R"({"test":1})");
}

TEST(ObjTest, one_level_multi)
{
    Json data;
    data["test"] = 1.0;
    EXPECT_TRUE(data.has("test"));
    data["test1"] = 2;
    EXPECT_EQ(data.type(), JSON_VALUE_OBJECT);
    EXPECT_EQ(data.dump(), R"({"test":1,"test1":2})");
}

// We forbid duplicate keys in operator[] method
// https://stackoverflow.com/questions/21832701/does-json-syntax-allow-duplicate-keys-in-an-object
TEST(ObjTest, duplicate_key)
{
    Json data;
    data["test"] = 1.0;
    data["test"] = 2;
    EXPECT_EQ(data.type(), JSON_VALUE_OBJECT);
    EXPECT_EQ(data.dump(), R"({"test":2})");
}

// We allow duplicate keys in push_back method
// A controversial topic, so we keep both rules
TEST(ObjTest, duplicate_key_push_back)
{
    Json data;
    data.push_back("test", 1.0);
    data.push_back("test", 2);
    EXPECT_EQ(data.type(), JSON_VALUE_OBJECT);
    EXPECT_EQ(data.dump(), R"({"test":1,"test":2})");
}

TEST(ObjTest, multi_level)
{
    Json data;
    data["test"]["test1"] = 1.0;
    EXPECT_EQ(data.type(), JSON_VALUE_OBJECT);
    EXPECT_EQ(data.dump(), R"({"test":{"test1":1}})");
}

TEST(ObjTest, multi_level_multi)
{
    Json data;
    data["test"]["test1"] = 1.0;
    data["test2"] = 2;
    EXPECT_EQ(data.type(), JSON_VALUE_OBJECT);
    EXPECT_EQ(data.dump(), R"({"test":{"test1":1},"test2":2})");
}

TEST(ObjTest, multi_level_not_match)
{
    Json data;
    data["test"] = 1;
    Json data_tmp = data["test"];
    EXPECT_TRUE(data_tmp.is_number());
    data["test"]["test1"] = 1.0;
    EXPECT_EQ(data.type(), JSON_VALUE_OBJECT);
    EXPECT_EQ(data.dump(), R"({"test":1})");
}

TEST(ObjTest, push_other_type)
{
    Json data;
    data.push_back("test1", false);
    data.push_back("test2", true);
    data.push_back("test3", "string");
    data.push_back("test4", nullptr);
    EXPECT_EQ(data.dump(),
              R"({"test1":false,"test2":true,"test3":"string","test4":null})");
}

TEST(ObjTest, other_type)
{
    Json data;
    data["test1"] = false;
    data["test2"] = true;
    data["test3"] = "string";
    data["test4"] = nullptr;
    EXPECT_EQ(data.dump(),
              R"({"test1":false,"test2":true,"test3":"string","test4":null})");
}

TEST(ObjTest, clear)
{
    Json data;
    EXPECT_TRUE(data.empty());
    data["test1"] = false;
    EXPECT_EQ(data.size(), 1);
    EXPECT_FALSE(data.empty());
    data.clear();
    EXPECT_EQ(data.size(), 0);
    EXPECT_TRUE(data.empty());
    EXPECT_EQ(data.type(), JSON_VALUE_OBJECT);
}

TEST(ObjTest, update)
{
    Json data;
    data["test1"] = false;
    EXPECT_EQ(data["test1"].get<bool>(), false);
    data["test1"] = true;
    EXPECT_EQ(data["test1"].get<bool>(), true);
    data["test1"] = 123;
    EXPECT_EQ(data["test1"].get<int>(), 123);
    data["test1"] = 11.0;
    EXPECT_EQ(data["test1"].get<double>(), 11.0);
    data["test1"] = "val";
    EXPECT_EQ(data["test1"].get<std::string>(), "val");
    data["test1"] = nullptr;
    EXPECT_EQ(data["test1"].get<std::nullptr_t>(), nullptr);
}

TEST(ObjTest, erase)
{
    Json data;
    data["test1"] = false;
    data["test2"] = true;
    data["test3"] = "string";
    data["test4"] = nullptr;
    EXPECT_EQ(data.dump(),
              R"({"test1":false,"test2":true,"test3":"string","test4":null})");
    data.erase("test2");
    EXPECT_EQ(data.dump(), R"({"test1":false,"test3":"string","test4":null})");
}

TEST(ObjTest, push_vector) {
  Json data;
  data.push_back("key1", {"val1", "val2"});
  data.push_back("key2", {"val3", "val4", "val5"});

  EXPECT_EQ(data["key1"][0].get<std::string>(), "val1");
  EXPECT_EQ(data["key1"][1].get<std::string>(), "val2");
  EXPECT_EQ(data["key2"][0].get<std::string>(), "val3");
  EXPECT_EQ(data["key2"][1].get<std::string>(), "val4");
  EXPECT_EQ(data["key2"][2].get<std::string>(), "val5");
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
