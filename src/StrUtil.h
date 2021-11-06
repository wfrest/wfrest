//
// Created by Chanchan on 11/6/21.
//

#ifndef _STRUTIL_H_
#define _STRUTIL_H_

#include "workflow/StringUtil.h"
#include <string>

namespace wfrest
{
    class StrUtil : public StringUtil
    {
    public:
        static std::string trim_pairs(const std::string &str, const char *pairs);

        static std::string trim(const std::string &str, const char *chars);

    };

}  // namespace wfrest


#endif //_STRUTIL_H_
