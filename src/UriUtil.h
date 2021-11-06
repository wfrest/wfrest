//
// Created by Chanchan on 11/4/21.
//

#ifndef _URIUTIL_H_
#define _URIUTIL_H_

#include "workflow/URIParser.h"
#include <unordered_map>
#include "StrUtil.h"

namespace wfrest
{

    class UriUtil : public URIParser
    {
    public:
        static std::unordered_map<std::string, std::string>
        split_query(const std::string &query);
    };

}  // wfrest

#endif //_URIUTIL_H_
