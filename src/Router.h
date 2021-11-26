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
    void handle(const char *route, const Handler &handler, int verb = GET);

    void call(const std::string &verb, const std::string &route, HttpReq *req, HttpResp *resp) const;

    static int parse_verb(const std::string &verb);

    void print_routes();   // for test

private:
    RouteTable routes_map_;
};

}  // wfrest

#endif // WFREST_ROUTER_H_
