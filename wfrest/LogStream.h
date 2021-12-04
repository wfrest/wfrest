// Modified from muduo

#ifndef WFREST_LOGSTREAM_H_
#define WFREST_LOGSTREAM_H_

#include <string>
#include <functional>
#include <utility>
#include <cassert>
#include "wfrest/StringPiece.h"

namespace wfrest
{
namespace detail
{
static constexpr size_t k_small_buf = 4000;
static constexpr size_t k_large_buf = 4000 * 1000;

template<int SIZE>
class FixedBuffer
{
public:
    FixedBuffer();

    ~FixedBuffer();

    FixedBuffer(const FixedBuffer &) = delete;

    FixedBuffer &operator=(const FixedBuffer &) = delete;

    bool append(const char *buf, size_t len);

    const char *data() const
    { return data_; }

    int length() const
    { return static_cast<int>(cur_ - data_); }

    char *current()
    { return cur_; }

    int available() const
    { return static_cast<int>(end() - cur_); }

    void add(size_t len)
    { cur_ += len; }

    void reset()
    { cur_ = data_; }

    void bzero()
    { memset(data_, 0, sizeof data_); }

    void set_cookie(void (*cookie)())
    { cookie_ = cookie; }

    // for used by GDB
    const char *debug_string();

    // for used by unit test
    std::string to_string() const
    { return string(data_, length()); }

    StringPiece to_string_piece() const
    { return StringPiece(data_, length()); }

private:
    const char *end() const
    { return data_ + sizeof data_; }

    // Must be outline function for cookies.
    static void cookie_start();

    static void cookie_end();

    // By looking for cookies in the core dump file (you can find messages that have not yet been written to disk
    // cookie is the address of the function
    void (*cookie_)();

    char data_[SIZE];
    char *cur_;
};

template<int SIZE>
FixedBuffer<SIZE>::FixedBuffer()
        : cur_(data_)
{
    set_cookie(FixedBuffer::cookie_start);
}

template<int SIZE>
FixedBuffer<SIZE>::~FixedBuffer()
{
    set_cookie(FixedBuffer::cookie_end);
}

template<int SIZE>
void FixedBuffer<SIZE>::cookie_start()
{
}

template<int SIZE>
void FixedBuffer<SIZE>::cookie_end()
{
}

template<int SIZE>
bool FixedBuffer<SIZE>::append(const char *buf, size_t len)
{
    if (static_cast<size_t>(available()) > len)
    {
        memcpy(cur_, buf, len);
        cur_ += len;
        return true;
    }
    return false;
}

template<int SIZE>
const char *FixedBuffer<SIZE>::debug_string()
{
    *cur_ = '\0';
    return data_;
}

}   // namespace detail

class LogStream
{
public:
    using self = LogStream;

    using Buffer = detail::FixedBuffer<detail::k_small_buf>;

    // overload stream opt <<
    // input data into FixedBuffer(not terminal or file ...)
    self &operator<<(bool v);

    self &operator<<(short);

    self &operator<<(unsigned short);

    self &operator<<(int);

    self &operator<<(unsigned int);

    self &operator<<(long);

    self &operator<<(unsigned long);

    self &operator<<(long long);

    self &operator<<(unsigned long long);

    self &operator<<(const void *);

    self &operator<<(float);

    self &operator<<(double);

    self &operator<<(char);

    self &operator<<(const char *);

    self &operator<<(const unsigned char *);

    self &operator<<(const std::string &);

    self &operator<<(const StringPiece &);

    self &operator<<(const Buffer &);

    void append(const char *data, int len)
    { buffer_.append(data, len); }

    const Buffer &buffer() const
    { return buffer_; }

    void reset_buf()
    { buffer_.reset(); }

private:
    template<typename T>
    void formatInteger(T);

private:
    Buffer buffer_;
    static const int k_max_numeric_size = 32;
};

class Fmt
{
public:
    template<typename T>
    Fmt(const char* fmt, T val);

    const char* data() const { return buf_; }
    int length() const { return length_; }

private:
    char buf_[32];
    int length_;
};

template<typename T>
Fmt::Fmt(const char* fmt, T val)
{
    static_assert(std::is_arithmetic<T>::value == true, "Must be arithmetic type");

    length_ = snprintf(buf_, sizeof buf_, fmt, val);
    assert(static_cast<size_t>(length_) < sizeof buf_);
}

inline LogStream& operator<<(LogStream& s, const Fmt& fmt)
{
    s.append(fmt.data(), fmt.length());
    return s;
}

}  // namespace wfrest

#endif // WFREST_LOGSTREAM_H_
