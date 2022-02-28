// Modified from drogon
// https://zlib.net/manual.html

#ifndef WFREST_COMPRESS_H_
#define WFREST_COMPRESS_H_

#include <string>
#include <zlib.h>

namespace wfrest
{

enum class Compress 
{
    GZIP
};

const char* compress_method_to_str(const Compress& compress_method);

class Compressor
{
public:
    static int gzip(const std::string * const src, std::string *dest);

    static int gzip(const char *data, const size_t len, std::string *dest);

    static int ungzip(const std::string * const src, std::string *dest);
    
    static int ungzip(const char *data, const size_t len, std::string *dest);
};

}  // namespace wfrest

#endif // WFREST_COMPRESS_H_
