//
// Created by Chanchan on 10/30/21.
//

#ifndef _ROUTER_H_
#define _ROUTER_H_

#include "RouteTable.h"
#include "WFHttpMsg.h"
#include <functional>

namespace wfrest
{

    class Router
    {
    public:
        Router() = default;
        enum { ANY, GET, POST, PUT, HTTP_DELETE };
        using Handler = std::function<void(HttpReq *, HttpResp *)>;

        struct VerbHandler
        {
            int verb = GET;
            Handler handler;
            std::string path;
        };

        void handle(std::string& route, const Handler& handler, int verb = GET);

    private:
        RouteTable<VerbHandler> routes_map_;
    };


}  // wfrest



#endif //_ROUTER_H_
