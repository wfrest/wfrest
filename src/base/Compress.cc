
#include <cassert>
#include "Compress.h"
#include "ErrorCode.h"

namespace wfrest
{
const char* compress_method_to_str(const Compress& compress_method)
{
    switch (compress_method)
    {
        case Compress::GZIP:
            return "gzip";
        default:
            return "unsupport compression";
    }
}
}  // namespace wfrest

using namespace wfrest;

int Compressor::gzip(const std::string * const src, std::string *dest)
{
    const char *data = src->c_str();
    const size_t len = src->size();
    return gzip(data, len, dest);
}

int Compressor::gzip(const char *data, const size_t len, std::string *dest)
{
    dest->clear();
    z_stream strm = {nullptr,
                     0,
                     0,
                     nullptr,
                     0,
                     0,
                     nullptr,
                     nullptr,
                     nullptr,
                     nullptr,
                     nullptr,
                     0,
                     0,
                     0};
    if (data && len > 0)
    {
        if (deflateInit2(&strm,
                         Z_DEFAULT_COMPRESSION,  
                         Z_DEFLATED,
                         MAX_WBITS + 16,
                         8,
                         Z_DEFAULT_STRATEGY) != Z_OK)
        {
            // fprintf(stderr, "deflateInit2 error!\n");
            return StatusCompressError;
        }
        std::string outstr;
        outstr.resize(compressBound(static_cast<uLong>(len)));
        strm.next_in = (Bytef *)data;
        strm.avail_in = static_cast<uInt>(len);
        int ret;
        do
        {
            if (strm.total_out >= outstr.size())
            {
                outstr.resize(strm.total_out * 2);
            }
            assert(outstr.size() >= strm.total_out);
            strm.avail_out = static_cast<uInt>(outstr.size() - strm.total_out);
            strm.next_out = (Bytef *)outstr.data() + strm.total_out;
            ret = deflate(&strm, Z_FINISH); /* no bad return value */
            if (ret == Z_STREAM_ERROR)
            {
                (void)deflateEnd(&strm);
                return StatusCompressError;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);
        assert(ret == Z_STREAM_END); /* stream will be complete */
        outstr.resize(strm.total_out);
        /* clean up and return */
        (void)deflateEnd(&strm);
        *dest = std::move(outstr);
        return StatusOK;
    }
    return StatusCompressError;
}
int Compressor::ungzip(const std::string * const src, std::string *dest)
{
    const char *data = src->c_str();
    const size_t len = src->size();
    return ungzip(data, len, dest);
}

int Compressor::ungzip(const char *data, const size_t len, std::string *dest)
{
    dest->clear();
    if (len == 0)
        return StatusOK;

    auto full_length = len;

    auto decompressed = std::string(full_length * 2, 0);
    bool done = false;

    z_stream strm = {nullptr,
                     0,
                     0,
                     nullptr,
                     0,
                     0,
                     nullptr,
                     nullptr,
                     nullptr,
                     nullptr,
                     nullptr,
                     0,
                     0,
                     0};
    strm.next_in = (Bytef *)data;
    strm.avail_in = static_cast<uInt>(len);
    strm.total_out = 0;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    if (inflateInit2(&strm, (15 + 32)) != Z_OK)
    {
        // fprintf(stderr, "inflateInit2 error!\n");
        return StatusUncompressError;
    }
    while (!done)
    {
        // Make sure we have enough room and reset the lengths.
        if (strm.total_out >= decompressed.length())
        {
            decompressed.resize(decompressed.length() * 2);
        }
        strm.next_out = (Bytef *)decompressed.data() + strm.total_out;
        strm.avail_out =
                static_cast<uInt>(decompressed.length() - strm.total_out);
        // Inflate another chunk.
        int status = inflate(&strm, Z_SYNC_FLUSH);
        if (status == Z_STREAM_END)
        {
            done = true;
        }
        else if (status != Z_OK)
        {
            break;
        }
    }
    if (inflateEnd(&strm) != Z_OK)
        return StatusUncompressError;
    // Set real length.
    int status = StatusOK;
    if (done)
    {
        decompressed.resize(strm.total_out);
        *dest = std::move(decompressed);
    }
    else
    {
        status = StatusUncompressError;
    }
    return status;
}

