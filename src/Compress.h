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

class Compressor
{
    static std::string gzip_compress(const char *data, const size_t len);

    std::string gzip_decompress(const char *data, const size_t len);

};
}  // namespace wfrest

#endif // WFREST_Compress_H_
