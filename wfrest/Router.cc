#include <arpa/inet.h>
#include "wfrest/Router.h"
#include "wfrest/HttpServerTask.h"
#include "wfrest/Logger.h"
#include "wfrest/HttpMsg.h"

using namespace wfrest;

namespace
{
static std::string get_peer_addr_str(HttpTask *server_task)
{
    static const int ADDR_STR_LEN = 128;
    char addrstr[ADDR_STR_LEN];
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof addr;
    unsigned short port = 0;

    server_task->get_peer_addr(reinterpret_cast<struct sockaddr *>(&addr), &addr_len);
    if (addr.ss_family == AF_INET)
    {
        auto *sin = reinterpret_cast<struct sockaddr_in *>(&addr);
        inet_ntop(AF_INET, &sin->sin_addr, addrstr, ADDR_STR_LEN);
        port = ntohs(sin->sin_port);
    } else if (addr.ss_family == AF_INET6)
    {
        auto *sin6 = reinterpret_cast<struct sockaddr_in6 *>(&addr);
        inet_ntop(AF_INET6, &sin6->sin6_addr, addrstr, ADDR_STR_LEN);
        port = ntohs(sin6->sin6_port);
    } else
        strcpy(addrstr, "Unknown");

    return addrstr;
}
}  // namespace
void Router::handle(const char *route, int compute_queue_id, const Handler &handler,
                    const SeriesHandler &series_handler, Verb verb)
{
    auto &vh = routes_map_[route];
    vh.verb = verb;
    vh.path = route;
    vh.handler = handler;
    vh.series_handler = series_handler;
    vh.compute_queue_id = compute_queue_id;
}

void Router::call(const std::string &verb, const std::string &route, HttpServerTask *server_task) const
{
    HttpReq *req = server_task->get_req();
    HttpResp *resp = server_task->get_resp();

    // skip the last / of the url.
    // /hello ==  /hello/
    StringPiece route2(route);
    if (!route2.empty() and route2[static_cast<int>(route2.size()) - 1] == '/')
        route2.remove_suffix(1);

    RouteParams route_params;
    std::string route_match_path;
    auto it = routes_map_.find(route2, route_params, route_match_path);

    if (it != routes_map_.end())   // has route
    {
        // match verb
        // it == <StringPiece : path, VerbHandler>
        if (it->second.verb == Verb::ANY or str_to_verb(verb) == it->second.verb)
        {
            req->set_full_path(it->second.path);
            req->set_route_params(std::move(route_params));
            req->set_route_match_path(std::move(route_match_path));
            server_task->add_callback([server_task, verb, route](HttpTask *)
                                      {
                                          LOG_INFO << "| " << get_peer_addr_str(server_task)
                                                   << " | " << verb << " : " << route << " |";
                                      });

            if (it->second.compute_queue_id == -1)
            {
                if (it->second.handler)
                    it->second.handler(req, resp);
                else
                    it->second.series_handler(req, resp, series_of(server_task));
            } else   // computing task
            {
                WFGoTask *go_task = WFTaskFactory::create_go_task(
                        "wfrest" + std::to_string(it->second.compute_queue_id),
                        it->second.handler,
                        req, resp);
                **server_task << go_task;
            }
        } else
        {
            resp->set_status(HttpStatusNotFound);
            LOG_ERROR << verb << " not implemented on route " << route2;
        }
    } else
    {
        resp->set_status(HttpStatusNotFound);
        LOG_ERROR << "Route dose not exist";
    }
}

void Router::print_routes() const
{
    routes_map_.all_routes([](const std::string &prefix, const VerbHandler &verb_handler)
                           {
                               fprintf(stderr, "[WFREST] %s\t/%s\n", verb_to_str(verb_handler.verb), prefix.c_str());
                           });
}

std::vector<std::pair<std::string, std::string> > Router::all_routes() const
{
    std::vector<std::pair<std::string, std::string> > res;
    routes_map_.all_routes([&res](const std::string &prefix, const VerbHandler &verb_handler)
                           {
                               res.emplace_back(verb_to_str(verb_handler.verb), prefix.c_str());
                           });
    return res;
}
