#ifndef WFREST_ROUTER_H_
#define WFREST_ROUTER_H_

#include <functional>
#include "RouteTable.h"
#include "Noncopyable.h"

namespace wfrest
{

class HttpServerTask;

class Router : public Noncopyable
{
public:
    void handle(const std::string &route, int compute_queue_id, const WrapHandler &handler, Verb verb);

    int call(Verb verb, const std::string &route, HttpServerTask *server_task) const;

    void print_routes() const;   // for logging

    std::vector<std::pair<std::string, std::string>> all_routes() const;   // for test 

    struct RouteVerb
    {
        std::string route;
        mutable std::set<Verb> verbs;
        bool operator()(const RouteVerb& l, const RouteVerb& r) const
        {
            return l.route > r.route;
        }
    };

    using RouteVerbIter = std::set<RouteVerb>::iterator;

    std::pair<RouteVerbIter, bool> add_route(Verb verb, const std::string &route);

    std::pair<RouteVerbIter, bool> add_route(const std::vector<Verb> &verbs, const std::string &route);
    
    void print_node_arch() { routes_map_.print_node_arch(); }

private:
    RouteTable routes_map_;
    std::set<RouteVerb, RouteVerb> routes_;  // for store 
    friend class BluePrint;
};

}  // wfrest

#endif // WFREST_ROUTER_H_
