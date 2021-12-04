// Modified from drogon
// https://zlib.net/manual.html

#ifndef WFREST_Compress_H_
#define WFREST_Compress_H_

#include <string>
#include <zlib.h>
#ifdef USE_BROTLI
#include <brotli/decode.h>
#include <brotli/encode.h>
#endif

namespace wfrest
{

enum class Compress 
{
    GZIP,
    BROTLI
};

const char* compress_method_to_str(const Compress& compress_method);

class Compressor
{
public:
    static std::string gzip(const char *data, const size_t len);

    static std::string ungzip(const char *data, const size_t len);

};

}  // namespace wfrest

#endif // WFREST_Compress_H_
