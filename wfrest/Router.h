#ifndef WFREST_ROUTER_H_
#define WFREST_ROUTER_H_

#include "workflow/HttpUtil.h"

#include <functional>

#include "wfrest/RouteTable.h"

namespace wfrest
{

class HttpServerTask;

class Router
{
public:
    void handle(const char *route, int compute_queue_id, const Handler &handler,
                const SeriesHandler &series_handler, Verb verb);

    void call(const std::string &verb, const std::string &route, HttpServerTask *server_task) const;

    void print_routes() const;   // for logging

    std::vector<std::pair<std::string, std::string>> all_routes() const;   // for test 

private:
    RouteTable routes_map_;

    friend class BluePrint;
};

}  // wfrest

#endif // WFREST_ROUTER_H_
