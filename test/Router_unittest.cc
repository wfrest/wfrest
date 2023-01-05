#include <vector>
#include <string>
#include <gtest/gtest.h>
#include "wfrest/Router.h"
#include "wfrest/VerbHandler.h"

using namespace wfrest;

using RegRoutes = std::vector<std::pair<std::string, std::string>>;

class RouterRegisterTest : public testing::Test
{
protected:
    // helper function
    void register_route(const std::string &route, const std::string &verb)
    {
        router_.handle(route.c_str(), 0, [](const HttpReq* , HttpResp* , SeriesWork *) -> WFGoTask * {
            return nullptr;
        }, str_to_verb(verb));
    }

    void register_route_list(const RegRoutes &routes_list)
    {
        for(const auto &r : routes_list)
        {
            register_route(r.first, r.second);
        }
    }
protected:
    Router router_;
};

// Becareful : /hello -> hello, we won't store the '/' at the front
// /api/v1/v2/ -> /api/v1/v2, we won't store the '/' at the back
TEST_F(RouterRegisterTest, reg_route)
{
    RegRoutes routes_list = {
        {"/hello", "GET"},
        {"/ping/", "POST"},
        {"/api/v1", "GET"},
        {"/api/v1/v2/", "GET"},
        {"/api/v1/v3/v4", "GET"},
        {"/api/{name}", "POST"},
        {"/api/action*", "GET"},
    };

    // register
    register_route_list(routes_list);

    // Becareful : /hello -> hello, we won't store the '/' at the front
    RegRoutes reg_list_exp = {
        {"GET", "api/action*"},
        {"GET", "api/v1/v2"},
        {"GET", "api/v1/v3/v4"},
        {"POST", "api/{name}"},
        {"GET", "hello"},
        {"POST", "ping"},
    };

    RegRoutes reg_list = router_.all_routes();
    EXPECT_EQ(reg_list_exp.size(), reg_list.size());
    for(int i = 0; i < reg_list.size(); i++)
    {
        EXPECT_EQ(reg_list_exp[i].first, reg_list[i].first);
        EXPECT_EQ(reg_list_exp[i].second, reg_list[i].second);
    }
}

TEST_F(RouterRegisterTest, root_route)
{
    RegRoutes routes_list = {
        {"/", "GET"},
        {"/ping/", "POST"},
    };

    // register
    register_route_list(routes_list);

    // Becareful : /hello -> hello, we won't store the '/' at the front
    RegRoutes reg_list_exp = {
        {"GET", "/"},
        {"POST", "ping"},
    };

    RegRoutes reg_list = router_.all_routes();
    EXPECT_EQ(reg_list_exp.size(), reg_list.size());
    for(int i = 0; i < reg_list.size(); i++)
    {
        EXPECT_EQ(reg_list_exp[i].first, reg_list[i].first);
        EXPECT_EQ(reg_list_exp[i].second, reg_list[i].second);
    }
}
