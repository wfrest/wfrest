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

};

} // namespace wfrest




#endif // WFREST_STATUSCODE_H_