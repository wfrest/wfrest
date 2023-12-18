
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

    StringPiece route3("/api/v1/chanchan/123/actiongogogo/test.css");
    it = rtn.find(route3, 0, route_params, route_match_path);
    EXPECT_TRUE(it != rtn.end());
    EXPECT_EQ(route_match_path, "actiongogogo/test.css");

    StringPiece route4("/api/v1/chanchan/123/actiongogogo/111/222/test.css");
    it = rtn.find(route4, 0, route_params, route_match_path);
    EXPECT_TRUE(it != rtn.end());
    EXPECT_EQ(route_match_path, "actiongogogo/111/222/test.css");

    StringPiece route5("/api/v1/chanchan/123/action");
    it = rtn.find(route5, 0, route_params, route_match_path);
    EXPECT_TRUE(it != rtn.end());
    EXPECT_EQ(route_match_path, "action");

    StringPiece route6("/api/v1/chanchan/123/actio11");
    it = rtn.find(route6, 0, route_params, route_match_path);
    EXPECT_TRUE(it == rtn.end());
}

TEST(RouteTableNode, find)
{
    RouteTableNode rtn;
    StringPiece route1("/api/test");
    rtn.find_or_create(route1, 0);

    std::map<std::string, std::string> route_params;
    std::string route_match_path;

    StringPiece route2("/api/test");
    RouteTableNode::iterator it = rtn.find(route2, 0, route_params, route_match_path);
    EXPECT_TRUE(it != rtn.end());

    // StringPiece route3("/api/test/");
    // it = rtn.find(route3, 0, route_params, route_match_path);
    // EXPECT_TRUE(it != rtn.end());

    StringPiece route4("/api/test/11");
    it = rtn.find(route4, 0, route_params, route_match_path);
    EXPECT_TRUE(it == rtn.end());
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

TEST(RouteTableNode, root_path_match)
{
    RouteTableNode rtn;
    StringPiece route1("/*");
    rtn.find_or_create(route1, 0);

    std::map<std::string, std::string> route_params;
    std::string route_match_path;

    StringPiece route2("/");
    RouteTableNode::iterator it = rtn.find(route2, 0, route_params, route_match_path);
    EXPECT_TRUE(it != rtn.end());
    EXPECT_EQ(route_match_path, "");

    StringPiece route3("/111");
    it = rtn.find(route3, 0, route_params, route_match_path);
    EXPECT_TRUE(it != rtn.end());
    EXPECT_EQ(route_match_path, "111");
}

