//
// Created by Chanchan on 10/30/21.
//

#include "Router.h"

using namespace wfrest;

void Router::handle(const char *route, const Handler &handler, int verb)
{
    auto &vh = routes_map_[route];
    vh.verb = verb;
    vh.path = route;
    vh.handler = handler;
}

void Router::call(const std::string &verb, const std::string &route, HttpReq *req, HttpResp *resp) const
{
    // skip the last / of the url.
    // /hello ==  /hello/
    fprintf(stderr, "route : %s\n", route.c_str());
    StringPiece route2(route);
    if (!route2.empty() and route2[route2.size() - 1] == '/')
        route2.remove_suffix(1);

    RouteParams route_params;
    auto it = routes_map_.find(route2, route_params);

    if (it != routes_map_.end())   // has route
    {
        // match verb
        // it == <StringPiece : path, VerbHandler>
        if (it->second.verb == ANY or parse_verb(verb) == it->second.verb)
        {
            req->set_route_params(std::move(route_params));
            it->second.handler(req, resp);
        } else
        {
            resp->set_status(HttpStatusNotFound);
            fprintf(stderr, "verb %s not implemented on route %s\n", verb.c_str(), route2.data());
        }
    } else
    {
        resp->set_status(HttpStatusNotFound);
        fprintf(stderr, "Route dose not exist\n");
    }
}

int Router::parse_verb(const std::string &verb)
{
    if (strcasecmp(verb.c_str(), "GET") == 0)
        return GET;
    if (strcasecmp(verb.c_str(), "PUT") == 0)
        return PUT;
    if (strcasecmp(verb.c_str(), "POST") == 0)
        return POST;
    if (strcasecmp(verb.c_str(), "HTTP_DELETE") == 0)
        return HTTP_DELETE;
    return ANY;
}

void Router::print_routes()
{
    routes_map_.all_routes([](const std::string &prefix, const VerbHandler &h)
                           {
                               fprintf(stderr, "%s\n", prefix.c_str());
                           });
//    routes_map_.all_routes([](auto r, auto h) { std::cout << r << '\n'; });
}
