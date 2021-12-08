#include <gtest/gtest.h>
#include <unordered_map>
#include "wfrest/Router.h"

using namespace wfrest;


// Becareful : /hello -> hello, we won't store the '/' at the front 
TEST(Router, all_routes_01)
{
    Router router;
    
    router.handle("/hello", 0, [](const HttpReq* req, HttpResp* resp){
        printf("world");
    }, nullptr, Verb::GET);

    std::vector<std::pair<std::string, std::string>> route_list = router.all_routes();

    EXPECT_EQ(route_list[0].first, "GET");
    EXPECT_EQ(route_list[0].second, "hello");
}

TEST(Router, all_routes_02)
{
    Router router;
    
    router.handle("/ping", 0, [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, nullptr, Verb::POST);


    std::vector<std::pair<std::string, std::string>> route_list = router.all_routes();

    EXPECT_EQ(route_list[0].first, "POST");
    EXPECT_EQ(route_list[0].second, "ping");
}

TEST(Router, all_routes_03)
{
    Router router;
    
    router.handle("/api/v1", 0, [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, nullptr, Verb::GET);

    std::vector<std::pair<std::string, std::string>> route_list = router.all_routes();

    EXPECT_EQ(route_list[0].first, "GET");
    EXPECT_EQ(route_list[0].second, "api/v1");
}

TEST(Router, all_routes_04)
{
    Router router;
    
    router.handle("/api/v1/v2/", 0, [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, nullptr, Verb::GET);

    std::vector<std::pair<std::string, std::string>> route_list = router.all_routes();

    EXPECT_EQ(route_list[0].first, "GET");
    EXPECT_EQ(route_list[0].second, "api/v1/v2/");
}


TEST(Router, all_routes_05)
{
    Router router;
    
    router.handle("/api/v1/v3/v4", 1, [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, nullptr, Verb::GET);
    std::vector<std::pair<std::string, std::string>> route_list = router.all_routes();

    EXPECT_EQ(route_list[0].first, "GET");
    EXPECT_EQ(route_list[0].second, "api/v1/v3/v4");
}

TEST(Router, all_routes_06)
{
    Router router;
    
    router.handle("/api/{name}", 0, [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, nullptr, Verb::GET);

    std::vector<std::pair<std::string, std::string>> route_list = router.all_routes();

    EXPECT_EQ(route_list[0].first, "GET");
    EXPECT_EQ(route_list[0].second, "api/{name}");
}

TEST(Router, all_routes_07)
{
    Router router;
    
    router.handle("/api/action*", 1, [](const HttpReq* req, HttpResp* resp){
        printf("pong");
    }, nullptr, Verb::GET);

    std::vector<std::pair<std::string, std::string>> route_list = router.all_routes();

    EXPECT_EQ(route_list[0].first, "GET");
    EXPECT_EQ(route_list[0].second, "api/action*");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}