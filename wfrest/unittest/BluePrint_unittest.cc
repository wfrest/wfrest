#include <gtest/gtest.h>
#include <unordered_map>
#include "wfrest/BluePrint.h"

using namespace wfrest;

BluePrint create_blue_print()
{
    BluePrint bp;
    bp.GET("name", [](const HttpReq* req, HttpResp* resp){
        printf("name login");
    });

    bp.POST("token", [](const HttpReq* req, HttpResp* resp){
        printf("token login");
    });
    return bp;
}

TEST(BluePrint, add_blueprint)
{
    BluePrint bp;
    bp.GET("/v1/v2", [](const HttpReq* req, HttpResp* resp){
        printf("v1");
    });

    BluePrint bp1 = create_blue_print();

    bp.add_blueprint(bp1, "/login");

    std::vector<std::pair<std::string, std::string>> route_list = bp.router().all_routes();
    EXPECT_EQ(route_list[0].first, "GET");
    EXPECT_EQ(route_list[0].second, "login/name");

    EXPECT_EQ(route_list[1].first, "POST");
    EXPECT_EQ(route_list[1].second, "login/token");

    EXPECT_EQ(route_list[2].first, "GET");
    EXPECT_EQ(route_list[2].second, "v1/v2");

    // bp.router().print_routes();
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}