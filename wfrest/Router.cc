#include "workflow/HttpUtil.h"

#include "wfrest/Router.h"
#include "wfrest/HttpServerTask.h"
#include "wfrest/HttpMsg.h"
#include "wfrest/ErrorCode.h"

using namespace wfrest;

void Router::handle(const char *route, int compute_queue_id, const WrapHandler &handler, Verb verb)
{
    VerbHandler &vh = routes_map_.find_or_create(route);
    if(vh.verb_set.find(verb) != vh.verb_set.end()) 
        return;
    vh.verb_set.insert(verb);
    vh.path = route;
    vh.handler = handler;
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
    if (route2.size() > 1 and route2[static_cast<int>(route2.size()) - 1] == '/')
        route2.remove_suffix(1);

    std::map<std::string, std::string> route_params;
    std::string route_match_path;
    auto it = routes_map_.find(route2, route_params, route_match_path);

    int error_code = StatusOK;
    if (it != routes_map_.end())   // has route
    {
        // match verb
        // it == <StringPiece : path, VerbHandler>
        std::set<Verb> &verb_set = it->second.verb_set;
        if(verb_set.find(Verb::ANY) != verb_set.end() or
            verb_set.find(verb) != verb_set.end())
        {
            req->set_full_path(it->second.path);
            req->set_route_params(std::move(route_params));
            req->set_route_match_path(std::move(route_match_path));

            WFGoTask * go_task = it->second.handler(req, resp, series_of(server_task));
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
    routes_map_.all_routes([](const std::string &prefix, const VerbHandler &verb_handler)
                        {
                            if(prefix == "/")
                            {
                                for(auto& verb : verb_handler.verb_set)
                                {
                                    fprintf(stderr, "[WFREST] %s\t%s\n", verb_to_str(verb), prefix.c_str());
                                }
                            }
                            else 
                            {
                                for(auto& verb : verb_handler.verb_set)
                                {
                                    fprintf(stderr, "[WFREST] %s\t/%s\n", verb_to_str(verb), prefix.c_str());
                                }
                            }
                        });
}

std::vector<std::pair<std::string, std::string> > Router::all_routes() const
{
    std::vector<std::pair<std::string, std::string> > res;
    routes_map_.all_routes([&res](const std::string &prefix, const VerbHandler &verb_handler)
                        {
                            for(auto& verb : verb_handler.verb_set)
                            {
                                res.emplace_back(verb_to_str(verb), prefix.c_str());
                            }
                            
                        });
    return res;
}
