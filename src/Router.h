#ifndef WFREST_ROUTER_H_
#define WFREST_ROUTER_H_

#include "VerbHandler.h"
#include "HttpMsg.h"
#include "Macro.h"
#include "RouteTable.h"
#include "workflow/HttpUtil.h"
#include <functional>

namespace wfrest
{

class Router
{
public:
    void handle(const char *route, int compute_queue_id, const Handler &handler,
                const SeriesHandler &series_handler, Verb verb);

    void call(const std::string &verb, const std::string &route, HttpReq *req, HttpResp *resp) const;

    static Verb str_to_verb(const std::string &verb);

    static const char *verb_to_str(const Verb &verb);

    void print_routes();   // for test
//    void all_routes();   // for test
private:
    RouteTable routes_map_;
};

}  // wfrest

#endif // WFREST_ROUTER_H_
