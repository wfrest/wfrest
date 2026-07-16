
#include <string>
#include <map>
#include <vector>
#include <gtest/gtest.h>
#include "wfrest/RouteTable.h"

using namespace wfrest;

namespace
{

void mark_route(VerbHandler &handler, Verb verb = Verb::GET)
{
    handler.verb_handler_map.emplace(verb, WrapHandler{});
}

} // namespace

TEST(RouteTableNode, create_and_find)
{
    RouteTableNode rtn;
    StringPiece route1("/api/v1/{name}/{passwd}/action*");
    mark_route(rtn.find_or_create(route1, 0));

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
    mark_route(rtn.find_or_create(route1, 0));

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
    mark_route(rtn.find_or_create(route1, 0));

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
    mark_route(rtn.find_or_create(route1, 0));

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

TEST(RouteTable, owns_caller_and_node_route_buffers)
{
    RouteTable table;
    RouteTableNode node;
    {
        std::string route = "/temporary/owned/path";
        mark_route(table.find_or_create(route.c_str()));
    }
    {
        std::string route = "/direct/node/path";
        mark_route(node.find_or_create(StringPiece(route), 0));
    }

    std::vector<std::string> churn(10000, std::string(128, 'x'));
    std::map<std::string, std::string> params;
    std::string match;
    EXPECT_NE(table.find(StringPiece("/temporary/owned/path"), params, match),
              table.end());
    EXPECT_NE(node.find(StringPiece("/direct/node/path"), 0, params, match),
              node.end());

    std::vector<std::string> routes;
    table.all_routes([&routes](const std::string &path, const VerbHandler &)
    {
        routes.push_back(path);
    });
    ASSERT_EQ(routes.size(), 1U);
    EXPECT_EQ(routes.front(), "temporary/owned/path");
}

TEST(RouteTable, backtracks_parameter_subtrees)
{
    RouteTable table;
    mark_route(table.find_or_create("/users/{id}/posts"), Verb::GET);
    mark_route(table.find_or_create("/users/{name}/profile"), Verb::POST);

    std::map<std::string, std::string> params;
    std::string match;
    const auto result = table.find(
        StringPiece("/users/alice/profile"), params, match);
    ASSERT_NE(result, table.end());
    EXPECT_EQ(result->second.verb_handler_map.count(Verb::POST), 1U);
    EXPECT_EQ(params.size(), 1U);
    EXPECT_EQ(params.at("name"), "alice");
}

TEST(RouteTable, failed_lookup_restores_outputs)
{
    RouteTable table;
    mark_route(table.find_or_create("/users/{id}/posts"));

    std::map<std::string, std::string> params = {
        {"id", "original"}, {"keep", "value"}};
    const auto original_params = params;
    std::string match = "original-match";
    const auto result = table.find(
        StringPiece("/users/alice/profile"), params, match);
    EXPECT_EQ(result, table.end());
    EXPECT_EQ(params, original_params);
    EXPECT_EQ(match, "original-match");
}

TEST(RouteTable, uses_static_parameter_wildcard_precedence)
{
    RouteTable table;
    mark_route(table.find_or_create("/files/static"), Verb::GET);
    mark_route(table.find_or_create("/files/{name}"), Verb::POST);
    mark_route(table.find_or_create("/files/s*"), Verb::PUT);

    std::map<std::string, std::string> params;
    std::string match;
    auto result = table.find(StringPiece("/files/static"), params, match);
    ASSERT_NE(result, table.end());
    EXPECT_EQ(result->second.verb_handler_map.count(Verb::GET), 1U);
    EXPECT_TRUE(params.empty());

    result = table.find(StringPiece("/files/something"), params, match);
    ASSERT_NE(result, table.end());
    EXPECT_EQ(result->second.verb_handler_map.count(Verb::POST), 1U);
    EXPECT_EQ(params.at("name"), "something");
}

TEST(RouteTable, chooses_longest_wildcard_prefix)
{
    RouteTable table;
    mark_route(table.find_or_create("/files/a*"), Verb::GET);
    mark_route(table.find_or_create("/files/ab*"), Verb::POST);

    std::map<std::string, std::string> params;
    std::string match;
    const auto result = table.find(StringPiece("/files/abc/rest"),
                                   params, match);
    ASSERT_NE(result, table.end());
    EXPECT_EQ(result->second.verb_handler_map.count(Verb::POST), 1U);
    EXPECT_EQ(match, "abc/rest");
}

TEST(RouteTable, rejects_empty_parameters_and_handlerless_nodes)
{
    RouteTable table;
    mark_route(table.find_or_create("/users/{ }"), Verb::GET);
    mark_route(table.find_or_create("/users/{id}"), Verb::POST);
    table.find_or_create("/empty");

    std::map<std::string, std::string> params;
    std::string match;
    auto result = table.find(StringPiece("/users/value"), params, match);
    ASSERT_NE(result, table.end());
    EXPECT_EQ(result->second.verb_handler_map.count(Verb::POST), 1U);
    EXPECT_EQ(params.at("id"), "value");

    params.clear();
    EXPECT_EQ(table.find(StringPiece("/users/"), params, match), table.end());
    EXPECT_EQ(table.find(StringPiece("/empty"), params, match), table.end());
}

TEST(RouteTable, enumerates_prefix_and_descendant_endpoints)
{
    RouteTable table;
    mark_route(table.find_or_create("/api/v1"), Verb::GET);
    mark_route(table.find_or_create("/api/v1/v2"), Verb::POST);
    mark_route(table.find_or_create("/api/v1"), Verb::PUT);

    std::map<std::string, size_t> routes;
    table.all_routes([&routes](const std::string &path,
                              const VerbHandler &handler)
    {
        routes[path] = handler.verb_handler_map.size();
    });

    ASSERT_EQ(routes.size(), 2U);
    EXPECT_EQ(routes.at("api/v1"), 2U);
    EXPECT_EQ(routes.at("api/v1/v2"), 1U);
}

TEST(RouteTable, handles_trailing_registration_slash_and_long_segments)
{
    RouteTable table;
    mark_route(table.find_or_create("/trailing/"), Verb::GET);

    const std::string long_segment(8192, 'x');
    const std::string long_route = "/long/" + long_segment;
    mark_route(table.find_or_create(long_route.c_str()), Verb::POST);

    std::map<std::string, std::string> params;
    std::string match;
    auto result = table.find(StringPiece("/trailing"), params, match);
    ASSERT_NE(result, table.end());
    EXPECT_EQ(result->second.verb_handler_map.count(Verb::GET), 1U);

    result = table.find(StringPiece(long_route), params, match);
    ASSERT_NE(result, table.end());
    EXPECT_EQ(result->second.verb_handler_map.count(Verb::POST), 1U);
}
