//
// Created by Chanchan on 10/30/21.
//

#include "Router.h"

using namespace wfrest;

void wfrest::Router::handle(std::string &&route, const Router::Handler &handler, int verb)
{
    auto &vh = routes_map_[route];
    vh.verb = verb;
    vh.path = std::move(route);
    vh.handler = handler;
}

void Router::call(const std::string &verb, const std::string &route, HttpReq *req, HttpResp *resp) const
{
    // skip the last / of the url.
    StringPiece route2(route);
    if (!route2.empty() and route2[route2.size() - 1] == '/')
        route2.remove_suffix(1);

    auto it = routes_map_.find(route2);
    if (it != routes_map_.end())   // has route
    {
        // match verb
        if (it->second.verb == ANY or parse_verb(verb) == it->second.verb)
        {
            it->second.handler(req, resp);
        } else
        {
            resp->set_status(HttpStatusNotFound);
            fprintf(stderr, "verb %s not implemented on route %s\n", verb.c_str(), route2.data());
        }
    } else
    {
        resp->set_status(HttpStatusNotFound);
        fprintf(stderr, "Route dose not exist");
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

