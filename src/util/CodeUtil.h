// Modified from cinatra

#ifndef WFREST_CODEUTIL_H_
#define WFREST_CODEUTIL_H_

#include <cstddef>
#include <string>

namespace wfrest
{
    
class StringPiece;

class CodeUtil
{
public:
    static std::string url_encode(const std::string &value);

    static std::string url_decode(const std::string &value);

    static bool is_url_encode(const std::string &str);
};

} // namespace wfrest


#endif // WFREST_CODEUTIL_H_
