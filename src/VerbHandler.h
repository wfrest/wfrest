#ifndef _VERBHANDLER_H_
#define _VERBHANDLER_H_

#include <functional>
#include "HttpMsg.h"

namespace wfrest
{
using Handler = std::function<void(HttpReq * , HttpResp * )>;

enum
{
    ANY, GET, POST, PUT, HTTP_DELETE
};

struct VerbHandler
{
    int verb = GET;
    Handler handler;
    std::string path;
};
}

#endif //_VERBHANDLER_H_
