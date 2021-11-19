//
// Created by Chanchan on 11/12/21.
//
// Taken from libhv

#ifndef _PATHUTIL_H_
#define _PATHUTIL_H_

#include <string>

namespace wfrest
{
class PathUtil
{
public:
    // filepath = /usr/local/image/test.jpg
    // base = test.jpg
    static std::string base(const std::string &filepath);
};
}  // namespace wfrest




#endif //_PATHUTIL_H_
