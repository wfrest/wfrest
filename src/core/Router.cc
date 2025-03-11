#include "workflow/HttpUtil.h"

#include "Router.h"
#include "HttpServerTask.h"
#include "HttpMsg.h"
#include "ErrorCode.h"
#include "CodeUtil.h"

using namespace wfrest;

void Router::handle(const std::string &route, int compute_queue_id, const WrapHandler &handler, Verb verb)
{
    std::pair<RouteVerbIter, bool> rv_pair = add_route(verb, route);
    VerbHandler &vh = routes_map_.find_or_create(rv_pair.first->route.c_str());
    if(vh.verb_handler_map.find(verb) != vh.verb_handler_map.end())
    {
        fprintf(stderr, "Duplicate Verb\n");
        return;
    }

    vh.verb_handler_map.insert({verb, handler});
    vh.path = rv_pair.first->route;
    vh.compute_queue_id = compute_queue_id;
}

int Router::call(Verb verb, const std::string &route, HttpServerTask *server_task) const
{
    HttpReq *req = server_task->get_req();
    HttpResp *resp = server_task->get_resp();

    // skip the last / of the url. Except for /
    // /hello ==  /hello/
    // / not change
    StringPiece route2(route);
    if (route2.size() > 1 && route2[static_cast<int>(route2.size()) - 1] == '/')
        route2.remove_suffix(1);

    std::map<std::string, std::string> route_params;
    std::string route_match_path;
    auto it = routes_map_.find(route2, route_params, route_match_path);

    int error_code = StatusOK;
    if (it != routes_map_.end())   // has route
    {
        // match verb
        // it == <StringPiece : path, VerbHandler>
        std::map<Verb, WrapHandler> &verb_handler_map = it->second.verb_handler_map;
        bool has_verb = (verb_handler_map.find(verb) != verb_handler_map.end());
        if (has_verb || verb_handler_map.find(Verb::ANY) != verb_handler_map.end())
        {
            req->set_full_path(it->second.path.as_string());
            req->set_route_params(std::move(route_params));
            req->set_route_match_path(std::move(route_match_path));
            WFGoTask * go_task;
            if(has_verb)
            {
                go_task = it->second.verb_handler_map[verb](req, resp, series_of(server_task));
            } else
            {
                go_task = it->second.verb_handler_map[Verb::ANY](req, resp, series_of(server_task));
            }
            if(go_task)
                **server_task << go_task;
        } else
        {
            error_code = StatusRouteVerbNotImplment;
        }
    } else
    {
        error_code = StatusRouteNotFound;
    }
    return error_code;
}

void Router::print_routes() const
{
    for(auto &rv : routes_)
    {
        for(auto verb : rv.verbs)
        {
            if (CodeUtil::is_url_encode(rv.route))
            {
                fprintf(stderr, "[WFREST] %s\t%s\n", verb_to_str(verb), CodeUtil::url_decode(rv.route).c_str());
            } else
            {
                fprintf(stderr, "[WFREST] %s\t%s\n", verb_to_str(verb), rv.route.c_str());
            }
        }
    }
}

std::vector<std::pair<std::string, std::string> > Router::all_routes() const
{
    std::vector<std::pair<std::string, std::string> > res;
    routes_map_.all_routes([&res](const std::string &prefix, const VerbHandler &verb_handler)
                        {
                            for(auto& vh : verb_handler.verb_handler_map)
                            {
                                res.emplace_back(verb_to_str(vh.first), prefix.c_str());
                            }
                        });
    return res;
}

std::pair<Router::RouteVerbIter, bool> Router::add_route(Verb verb, const std::string &route)
{
    RouteVerb rv;
    rv.route = route;
    auto it = routes_.find(rv);
    if(it != routes_.end())
    {
        it->verbs.insert(verb);
    } else
    {
        rv.verbs.insert(verb);
    }
    return routes_.emplace(std::move(rv));
}

std::pair<Router::RouteVerbIter, bool> Router::add_route(const std::vector<Verb> &verbs, const std::string &route)
{
    RouteVerb rv;
    rv.route = route;
    auto it = routes_.find(rv);
    if(it != routes_.end())
    {
        for(auto verb : verbs)
            it->verbs.insert(verb);
    } else
    {
        for(auto verb : verbs)
            rv.verbs.insert(verb);
    }
    return routes_.emplace(std::move(rv));
}
