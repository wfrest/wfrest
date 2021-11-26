#ifndef WFREST_URIUTIL_H_
#define WFREST_URIUTIL_H_

#include "workflow/URIParser.h"
#include <unordered_map>
#include "StrUtil.h"

namespace wfrest
{

class UriUtil : public URIParser
{
public:
    static std::unordered_map<std::string, std::string>
    split_query(const StringPiece &query);
};

}  // wfrest

#endif // WFREST_URIUTIL_H_
