#ifndef WFREST_VERBHANDLER_H_
#define WFREST_VERBHANDLER_H_

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
}  // namespace wfrest

#endif // WFREST_VERBHANDLER_H_
