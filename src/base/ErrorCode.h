#ifndef WFREST_STATUSCODE_H_
#define WFREST_STATUSCODE_H_

namespace wfrest
{

enum ErrorCode
{
    StatusOK = 0,
    StatusNotFound,
    // compress 
    StatusCompressError,
    StatusCompressNotSupport,
    StatusNoComrpess,
    StatusUncompressError,
    StatusUncompressNotSupport,
    StatusNoUncomrpess,

    // File
    StatusFileRangeInvalid,
    StatusFileReadError,
    StatusFileWriteError,

    // Json
    StatusJsonInvalid,
    
    StatusProxyError,

    // Route
    StatusRouteVerbNotImplment,
    StatusRouteNotFound,
};

const char* error_code_to_str(int code);

} // namespace wfrest

#endif // WFREST_STATUSCODE_H_