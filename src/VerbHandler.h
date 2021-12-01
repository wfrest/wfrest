#ifndef WFREST_VERBHANDLER_H_
#define WFREST_VERBHANDLER_H_

#include <functional>
#include "HttpMsg.h"

namespace wfrest
{
using Handler = std::function<void(HttpReq * , HttpResp *)>;
using SeriesHandler = std::function<void(HttpReq * , HttpResp *, SeriesWork *)>;

enum class Verb
{
    ANY, GET, POST, PUT, HTTP_DELETE
};

struct VerbHandler
{
    Verb verb = Verb::GET;
    Handler handler;
    SeriesHandler series_handler;
    std::string path;
    int compute_queue_id;
};

}  // namespace wfrest

#endif // WFREST_VERBHANDLER_H_
