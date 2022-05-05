#ifndef WFREST_VERBHANDLER_H_
#define WFREST_VERBHANDLER_H_

#include <functional>
#include <set>
#include "HttpMsg.h"

namespace wfrest
{

enum class Verb
{
    ANY, GET, POST, PUT, DELETE, HEAD, PATCH,
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
    if (strcasecmp(verb.c_str(), "HEAD") == 0)
        return Verb::HEAD;
    if (strcasecmp(verb.c_str(), "PATCH") == 0)
        return Verb::PATCH;
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
        case Verb::HEAD:
            return "HEAD";
        case Verb::PATCH:
            return "PATCH";
        default:
            return "[UNKNOWN]";
    }
}

using WrapHandler = std::function<WFGoTask *(HttpReq * , HttpResp *, SeriesWork *)>;

struct VerbHandler
{
    std::map<Verb, WrapHandler> verb_handler_map;
    StringPiece path;
    int compute_queue_id;
};

}  // namespace wfrest

#endif // WFREST_VERBHANDLER_H_
