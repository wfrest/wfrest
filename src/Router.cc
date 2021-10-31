//
// Created by Chanchan on 10/30/21.
//

#include "Router.h"
using namespace wfrest;

void wfrest::Router::handle(std::string& route, const Router::Handler &handler, int verb)
{
    auto& vh = routes_map_[route];
    vh.verb = verb;
    vh.path = std::move(route);
    vh.handler = handler;
}
