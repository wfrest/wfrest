#include <algorithm>
#include "LogStream.h"

using namespace wfrest;
using namespace wfrest::detail;

namespace
{
const char digits[] = "9876543210123456789";
// The two sides of zero are symmetric, because the remainder may be negative
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

const char digitsHex[] = "0123456789ABCDEF";
static_assert(sizeof digitsHex == 17, "wrong number of digitsHex");

size_t convertHex(char buf[], uintptr_t value)
{
    uintptr_t i = value;
    char *p = buf;

    do
    {
        int lsd = static_cast<int>(i % 16);
        i /= 16;
        *p++ = digitsHex[lsd];
    } while (i != 0);

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}
}  // namespace


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
    if (buffer_.available() >= k_max_numeric_size)
    {
        size_t len = convert(buffer_.current(), v);
        buffer_.add(len);
    }
}

LogStream::self &LogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}

LogStream::self &LogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}

LogStream::self &LogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}

LogStream::self &LogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}

LogStream::self &LogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

LogStream::self &LogStream::operator<<(const void *p)
{
    auto v = reinterpret_cast<uintptr_t>(p);
    if (buffer_.available() >= k_max_numeric_size)
    {
        char *buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = convertHex(buf + 2, v);
        buffer_.add(len + 2);
    }
    return *this;
}

LogStream::self &LogStream::operator<<(float v)
{
    *this << static_cast<double>(v);
    return *this;
}

LogStream::self &LogStream::operator<<(double v)
{
    if (buffer_.available() >= k_max_numeric_size)
    {
        int len = snprintf(buffer_.current(), k_max_numeric_size, "%.12g", v);
        buffer_.add(len);
    }
    return *this;
}

LogStream::self &LogStream::operator<<(char v)
{
    buffer_.append(&v, 1);
    return *this;
}

LogStream::self &LogStream::operator<<(const char *str)
{
    if (str)
    {
        buffer_.append(str, strlen(str));
    } else
    {
        buffer_.append("(null)", 6);
    }
    return *this;
}

LogStream::self &LogStream::operator<<(const unsigned char *str)
{
    return operator<<(reinterpret_cast<const char *>(str));
}

LogStream::self &LogStream::operator<<(const std::string &str)
{
    buffer_.append(str.c_str(), str.size());
    return *this;
}

LogStream::self &LogStream::operator<<(const StringPiece &str)
{
    buffer_.append(str.data(), str.size());
    return *this;
}

LogStream::self &LogStream::operator<<(const LogStream::Buffer &buf)
{
    *this << buf.to_string_piece();
    return *this;
}

void LogStream::static_check()
{
    static_assert(k_max_numeric_size - 10 > std::numeric_limits<double>::digits10,
                  "k_max_num_size is large enough");
    static_assert(k_max_numeric_size - 10 > std::numeric_limits<long double>::digits10,
                  "k_max_num_size is large enough");
    static_assert(k_max_numeric_size - 10 > std::numeric_limits<long>::digits10,
                  "k_max_num_size is large enough");
    static_assert(k_max_numeric_size - 10 > std::numeric_limits<long long>::digits10,
                  "k_max_num_size is large enough");
}

// Explicit instantiations Fmt
template Fmt::Fmt(const char* fmt, char);

template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);

template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);



















