//
// Created by Chanchan on 10/30/21.
//

#ifndef _ROUTER_H_
#define _ROUTER_H_

#include "RouteTable.h"
#include <functional>

template <typename Req, typename Resp>
class Router
{
public:
    enum { ANY, GET, POST, PUT, HTTP_DELETE };
    using Handler = std::function<void(Req&, Resp&)>;

    struct VerbHandler {
        int verb = ANY;
        Handler handler;
        std::string path;
    };

private:
//    RouteTable<VerbHandler> routes_map_;
};


#endif //_ROUTER_H_
