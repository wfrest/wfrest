#ifndef WFREST_STATUSCODE_H_
#define WFREST_STATUSCODE_H_

namespace wfrest
{

enum StatusCode
{
    StatusOK = 0,
    // compress 
    StatusCompressError,
    StatusCompressNotSupport,
    StatusNoComrpess,
    StatusUncompressError,
    StatusUncompressNotSupport,
    StatusNoUncomrpess,

    // File
    StatusFileNotFound,
    StatusFileRangeInvalid,
    StatusFileReadError,
};

const char* status_code_to_str(int code);

} // namespace wfrest




#endif // WFREST_STATUSCODE_H_