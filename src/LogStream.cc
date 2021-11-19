//
// Created by Chanchan on 11/19/21.
//

#include <algorithm>
#include "LogStream.h"

using namespace wfrest;
using namespace wfrest::detail;

namespace wfrest
{
namespace detail
{

const char digits[] = "9876543210123456789";
const char *zero = digits + 9;
static_assert(sizeof(digits) == 20, "wrong number of digits");

// Efficient Integer to String Conversions, by Matthew Wilson.
template<typename T>
size_t convert(char buf[], T value)
{
    T i = value;
    char *p = buf;

    do
    {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd];
    } while (i != 0);

    if (value < 0)
    {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

}  // namespace detail
}  // namespace wfrest

template<int SIZE>
FixedBuffer<SIZE>::FixedBuffer()
        : cur_(data_)
{
    set_cookie(cookieStart);
}

template<int SIZE>
FixedBuffer<SIZE>::~FixedBuffer()
{
    set_cookie(cookieEnd);
}

template<int SIZE>
void FixedBuffer<SIZE>::append(const char *buf, size_t len)
{
    if (static_cast<size_t>(avail()) > len)
    {
        memcpy(cur_, buf, len);
        cur_ += len;
    }
}

template<int SIZE>
const char *FixedBuffer<SIZE>::debug_string()
{
    *cur_ = '\0';
    return data_;
}

template<int SIZE>
void FixedBuffer<SIZE>::cookieStart()
{
}

template<int SIZE>
void FixedBuffer<SIZE>::cookieEnd()
{
}

LogStream::self &LogStream::operator<<(bool v)
{
    buffer_.append(v ? "1" : "0", 1);
    return *this;
}

LogStream::self &LogStream::operator<<(short v)
{
    *this << static_cast<int>(v);
    return *this;
}

LogStream::self &LogStream::operator<<(unsigned short v)
{
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream::self &LogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}

template<typename T>
void LogStream::formatInteger(T v)
{
    if (buffer_.avail() >= k_max_num_size)
    {
        size_t len = convert(buffer_.current(), v);
        buffer_.add(len);
    }
}






