#include "ErrorCode.h"
#include <map>

namespace wfrest
{

std::map<int, const char *> error_code_table = {
    { StatusOK, "OK" },
    { StatusCompressError, "Compress Error" },
    { StatusCompressNotSupport, "Compress Not Support" },
    { StatusNoComrpess, "No Comrpession" },
    { StatusUncompressError, "Uncompress Error" },
    { StatusUncompressNotSupport, "Uncompress Not Support" },
    { StatusNoUncomrpess, "No Uncomrpess" },
    { StatusNotFound, "404 Not Found" },
    { StatusFileRangeInvalid, "File Range Invalid" },
    { StatusFileReadError, "File Read Error" },
    { StatusFileWriteError, "File Write Error" },
    { StatusJsonInvalid, "Invalid Json Syntax" },
    { StatusProxyError, "Http Proxy Error" },
    { StatusRouteVerbNotImplment, "Route Http Method not implement" },
    { StatusRouteNotFound, "Route Not Found" },
};
 
const char* error_code_to_str(int code)
{
    auto it = error_code_table.find(code);
    if(it == error_code_table.end())
        return "unknown";
    return it->second;
}
    
} // namespace wfrest


