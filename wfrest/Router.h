#ifndef WFREST_ROUTER_H_
#define WFREST_ROUTER_H_

#include "workflow/HttpUtil.h"

#include <functional>

#include "wfrest/VerbHandler.h"
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

    void add_sub_router(std::string &&prefix, const Router& sub_router);

    static Verb str_to_verb(const std::string &verb);

    static const char *verb_to_str(const Verb &verb);

    void print_routes();   // for logging

    std::vector<std::pair<std::string, std::string>> all_routes();   // for test
private:
    RouteTable routes_map_;
};

}  // wfrest

#endif // WFREST_ROUTER_H_
