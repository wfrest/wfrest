#include "wfrest/StatusCode.h"
#include <map>

namespace wfrest
{

std::map<int, const char *> status_code_table = {
    { StatusOK, "OK" },
    { StatusCompressError, "Compress Error" },
    { StatusCompressNotSupport, "Compress Not Support" },
    { StatusNoComrpess, "No Comrpession" },
    { StatusUncompressError, "Uncompress Error" },
    { StatusUncompressNotSupport, "Uncompress Not Support" },
    { StatusNoUncomrpess, "No Uncomrpess" },
    { StatusFileNotFound, "File Not Found" },
    { StatusFileRangeInvalid, "File Range Invalid" },
    { StatusFileReadError, "File Read Error" },
    { StatusFileWriteError, "File Write Error" },
};
 
const char* status_code_to_str(int code)
{
    auto it = status_code_table.find(code);
    if(it == status_code_table.end())
        return "unknown";
    return it->second;
}
    
} // namespace wfrest


