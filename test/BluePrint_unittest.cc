#include <gtest/gtest.h>
#include <unordered_map>
#include "wfrest/BluePrint.h"
#include "wfrest/Router.h"

using namespace wfrest;

void set_login_bp(BluePrint &bp)
{
    bp.GET("name", [](const HttpReq* req, HttpResp* resp){
        printf("name login");
    });

    bp.POST("token", [](const HttpReq* req, HttpResp* resp){
        printf("token login");
    });
}

TEST(BluePrint, add_blueprint)
{
    BluePrint bp;
    bp.GET("/v1/v2", [](const HttpReq* req, HttpResp* resp){
        printf("v1");
    });

    BluePrint login_bp;
    set_login_bp(login_bp);

    bp.add_blueprint(login_bp, "/login");

    std::vector<std::pair<std::string, std::string>> route_list = bp.router().all_routes();
    EXPECT_EQ(route_list[0].first, "GET");
    EXPECT_EQ(route_list[0].second, "login/name");

    EXPECT_EQ(route_list[1].first, "POST");
    EXPECT_EQ(route_list[1].second, "login/token");

    EXPECT_EQ(route_list[2].first, "GET");
    EXPECT_EQ(route_list[2].second, "v1/v2");

    // bp.router().print_routes();
}

TEST(BluePrint, ROUTE)
{
    BluePrint bp;
    bp.ROUTE("/v1/v2", [](const HttpReq* req, HttpResp* resp){
        printf("v1");
    }, {"GET", "POST"});

    bp.router().print_routes();
    std::vector<std::pair<std::string, std::string>> route_list = bp.router().all_routes();
    EXPECT_EQ(route_list[0].first, "GET");
    EXPECT_EQ(route_list[1].first, "POST");
}
