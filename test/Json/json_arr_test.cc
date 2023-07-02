#include "Json.h"
#include <gtest/gtest.h>

using namespace wfrest;

TEST(ArrTest, create_arr)
{
    Json data = Json::Array{1, true, "string", nullptr, "123"};
    EXPECT_EQ(data.dump(), R"([1,true,"string",null,"123"])");
}

TEST(ArrTest, empty_arr)
{
    Json data = Json::Array();
    EXPECT_TRUE(data.is_array());
    EXPECT_EQ(data.dump(), "[]");
}

TEST(ArrTest, arr_push)
{
    Json data;
    data.push_back(1);
    data.push_back(nullptr);
    data.push_back("string");
    data.push_back(true);
    data.push_back(false);
    EXPECT_EQ(data.dump(), R"([1,null,"string",true,false])");
}

TEST(ArrTest, arr_update)
{
    Json data;
    data.push_back(1);
    data.push_back(nullptr);
    data.push_back("string");
    data.push_back(true);
    data.push_back(false);
    EXPECT_EQ(data.dump(), R"([1,null,"string",true,false])");
    data[2] = 1;
    EXPECT_EQ(data[2].get<int>(), 1);
    data[2] = 2.0;
    EXPECT_EQ(data[2].get<double>(), 2.0);
    data[2] = "123";
    EXPECT_EQ(data[2].get<std::string>(), "123");
    data[2] = nullptr;
    EXPECT_EQ(data[2].get<std::nullptr_t>(), nullptr);
}

TEST(ArrTest, arr_search)
{
    Json data;
    data.push_back(1);        // 0
    data.push_back(2.1);      // 1
    data.push_back(nullptr);  // 2
    data.push_back("string"); // 3
    data.push_back(true);     // 4
    data.push_back(false);    // 5

    EXPECT_EQ(data[0].get<int>(), 1);
    EXPECT_EQ(data[1].get<double>(), 2.1);
    EXPECT_EQ(data[2].get<std::nullptr_t>(), nullptr);
    EXPECT_EQ(data[3].get<std::string>(), "string");
    EXPECT_EQ(data[4].get<bool>(), true);
    EXPECT_EQ(data[5].get<bool>(), false);

    // Object
    Json::Object obj;
    obj["123"] = 12;
    obj["123"]["1"] = "test";
    // todo : we need a move interface
    // we copy here
    data.push_back(obj); // 6

    // std::cout << data[6] << std::endl;
    // std::cout << data[6].get<Json::Object>().dump() << std::endl;
    EXPECT_EQ(data[6].get<Json::Object>().dump(), R"({"123":12})");

    // Array
    Json::Array arr;
    arr.push_back(1);
    arr.push_back(nullptr);

    data.push_back(arr);

    // std::cout << data[7] << std::endl;
    // std::cout << data[7].get<Json::Array>().dump() << std::endl;
    EXPECT_EQ(data[7].get<Json::Array>().dump(), R"([1,null])");

    // implicit conversion
    int a = data[0];
    EXPECT_EQ(a, 1);
    double b = data[1];
    EXPECT_EQ(b, 2.1);
    std::nullptr_t c = data[2];
    EXPECT_EQ(c, nullptr);
    std::string d = data[3];
    EXPECT_EQ(d, "string");
    bool e = data[4];
    EXPECT_EQ(e, true);
    bool f = data[5];
    EXPECT_EQ(f, false);

    Json::Object g = data[6];
    EXPECT_EQ(g.dump(), R"({"123":12})");

    Json::Array h = data[7];
    EXPECT_EQ(h.dump(), R"([1,null])");
}

TEST(ArrTest, erase)
{
    Json data;
    data.push_back(1);
    data.push_back(nullptr);
    data.push_back("string");
    data.push_back(true);
    data.push_back(false);
    EXPECT_EQ(data.dump(), R"([1,null,"string",true,false])");
    data.erase(2);
    EXPECT_EQ(data.dump(), R"([1,null,true,false])");
}

TEST(ArrTest, push_vector) {
    Json data;
    std::vector<std::string> values = {"val1", "val2"};
    data.push_back(values);

    EXPECT_EQ(data[0].get<std::string>(), "val1");
    EXPECT_EQ(data[1].get<std::string>(), "val2");

    data.push_back({"val3", "val4", "val5"});
    EXPECT_EQ(data[2].get<std::string>(), "val3");
    EXPECT_EQ(data[3].get<std::string>(), "val4");
    EXPECT_EQ(data[4].get<std::string>(), "val5");
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
