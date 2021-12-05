#ifndef WFREST_URIUTIL_H_
#define WFREST_URIUTIL_H_

#include "workflow/URIParser.h"
#include <unordered_map>

namespace wfrest
{

class StringPiece;

class UriUtil : public URIParser
{
public:
    static std::map<std::string, std::string>
    split_query(const StringPiece &query);
};

}  // wfrest

#endif // WFREST_URIUTIL_H_
