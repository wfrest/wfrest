
#include <string>
#include <map>
#include <gtest/gtest.h>
#include "wfrest/RouteTable.h"

using namespace wfrest;

TEST(RouteTableNode, create_and_find)
{
    RouteTableNode rtn;
    StringPiece route1("/api/v1/{name}/{passwd}/action*");
    rtn.find_or_create(route1, 0);

    std::map<std::string, std::string> route_params;
    std::string route_match_path;

    StringPiece route2("/api/v1/chanchan/123/actiongogogo");
    RouteTableNode::iterator it = rtn.find(route2, 0, route_params, route_match_path);
    EXPECT_TRUE(it != rtn.end());
    EXPECT_EQ(route_params["name"], "chanchan");
    EXPECT_EQ(route_params["passwd"], "123");
    EXPECT_EQ(route_match_path, "actiongogogo");
}

TEST(RouteTableNode, root_path)
{
    RouteTableNode rtn;
    StringPiece route1("/");
    rtn.find_or_create(route1, 0);

    std::map<std::string, std::string> route_params;
    std::string route_match_path;

    StringPiece route2("/");
    RouteTableNode::iterator it = rtn.find(route2, 0, route_params, route_match_path);
    EXPECT_TRUE(it != rtn.end());
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}