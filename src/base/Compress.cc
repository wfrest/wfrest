#include "Compress.h"

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

#include "ErrorCode.h"

namespace wfrest
{

namespace
{

constexpr size_t kOutputBufferSize = 16 * 1024;

int fail(std::string *dest, int status)
{
    if (dest != nullptr)
        dest->clear();
    return status;
}

void assign_input(z_stream *stream,
                  const char *data,
                  size_t len,
                  size_t *offset)
{
    if (stream->avail_in != 0 || *offset == len)
        return;

    const size_t amount = std::min(
        len - *offset,
        static_cast<size_t>(std::numeric_limits<uInt>::max()));
    stream->next_in = reinterpret_cast<Bytef *>(
        const_cast<char *>(data + *offset));
    stream->avail_in = static_cast<uInt>(amount);
    *offset += amount;
}

size_t produced_size(const z_stream& stream)
{
    return kOutputBufferSize - stream.avail_out;
}

} // namespace

const char *compress_method_to_str(const Compress& compress_method)
{
    switch (compress_method)
    {
        case Compress::GZIP:
            return "gzip";
        default:
            return "unsupport compression";
    }
}

int Compressor::gzip(const std::string * const src, std::string *dest)
{
    if (dest == nullptr)
        return StatusCompressError;
    if (src == nullptr)
        return fail(dest, StatusCompressError);
    return gzip(src->c_str(), src->size(), dest);
}

int Compressor::gzip(const char *data, const size_t len, std::string *dest)
{
    if (dest == nullptr)
        return StatusCompressError;
    if (data == nullptr && len != 0)
        return fail(dest, StatusCompressError);

    z_stream stream{};
    if (deflateInit2(&stream,
                     Z_DEFAULT_COMPRESSION,
                     Z_DEFLATED,
                     MAX_WBITS + 16,
                     8,
                     Z_DEFAULT_STRATEGY) != Z_OK)
    {
        return fail(dest, StatusCompressError);
    }

    std::array<char, kOutputBufferSize> buffer{};
    std::string output;
    size_t offset = 0;
    int status = Z_OK;

    while (status != Z_STREAM_END)
    {
        assign_input(&stream, data, len, &offset);
        const int flush = offset == len && stream.avail_in == 0
                          ? Z_FINISH
                          : Z_NO_FLUSH;

        do
        {
            stream.next_out = reinterpret_cast<Bytef *>(buffer.data());
            stream.avail_out = static_cast<uInt>(buffer.size());
            status = deflate(&stream, flush);
            if (status != Z_OK && status != Z_STREAM_END)
            {
                (void)deflateEnd(&stream);
                return fail(dest, StatusCompressError);
            }

            output.append(buffer.data(), produced_size(stream));
        } while (stream.avail_out == 0);
    }

    if (deflateEnd(&stream) != Z_OK)
        return fail(dest, StatusCompressError);

    *dest = std::move(output);
    return StatusOK;
}

int Compressor::ungzip(const std::string * const src, std::string *dest)
{
    if (dest == nullptr)
        return StatusUncompressError;
    if (src == nullptr)
        return fail(dest, StatusUncompressError);
    return ungzip(src->c_str(), src->size(), dest);
}

int Compressor::ungzip(const char *data, const size_t len, std::string *dest)
{
    if (dest == nullptr)
        return StatusUncompressError;
    if (len == 0)
    {
        dest->clear();
        return StatusOK;
    }
    if (data == nullptr)
        return fail(dest, StatusUncompressError);

    z_stream stream{};
    if (inflateInit2(&stream, 15 + 32) != Z_OK)
        return fail(dest, StatusUncompressError);

    std::array<char, kOutputBufferSize> buffer{};
    std::string output;
    size_t offset = 0;
    int status = Z_OK;

    while (status != Z_STREAM_END)
    {
        assign_input(&stream, data, len, &offset);
        stream.next_out = reinterpret_cast<Bytef *>(buffer.data());
        stream.avail_out = static_cast<uInt>(buffer.size());

        status = inflate(&stream, Z_NO_FLUSH);
        const size_t produced = produced_size(stream);
        output.append(buffer.data(), produced);

        if (status == Z_STREAM_END)
            break;
        if (status != Z_OK ||
            (produced == 0 && stream.avail_in == 0 && offset == len))
        {
            (void)inflateEnd(&stream);
            return fail(dest, StatusUncompressError);
        }
    }

    if (inflateEnd(&stream) != Z_OK)
        return fail(dest, StatusUncompressError);

    *dest = std::move(output);
    return StatusOK;
}

} // namespace wfrest
