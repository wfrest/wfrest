#ifndef WFREST_VERBHANDLER_H_
#define WFREST_VERBHANDLER_H_

#include <functional>
#include "wfrest/HttpMsg.h"

namespace wfrest
{

using Handler = std::function<void(HttpReq * , HttpResp *)>;
using SeriesHandler = std::function<void(HttpReq * , HttpResp *, SeriesWork *)>;

enum class Verb
{
    ANY, GET, POST, PUT, DELETE
};

inline Verb str_to_verb(const std::string &verb)
{
    if (strcasecmp(verb.c_str(), "GET") == 0)
        return Verb::GET;
    if (strcasecmp(verb.c_str(), "PUT") == 0)
        return Verb::PUT;
    if (strcasecmp(verb.c_str(), "POST") == 0)
        return Verb::POST;
    if (strcasecmp(verb.c_str(), "DELETE") == 0)
        return Verb::DELETE;
    return Verb::ANY;
}

inline const char *verb_to_str(const Verb &verb)
{
    switch (verb)
    {
        case Verb::ANY:
            return "ANY";
        case Verb::GET:
            return "GET";
        case Verb::POST:
            return "POST";
        case Verb::PUT:
            return "PUT";
        case Verb::DELETE:
            return "DELETE";
        default:
            return "[UNKNOWN]";
    }
}

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
