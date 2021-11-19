//
// Created by Chanchan on 11/19/21.
//

#ifndef _LOGSTREAM_H_
#define _LOGSTREAM_H_

#include <string>
#include <functional>
#include <utility>
#include "StringPiece.h"

namespace wfrest
{
namespace detail
{
const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template<int SIZE>
class FixedBuffer
{
public:
    FixedBuffer();

    ~FixedBuffer();

    FixedBuffer(const FixedBuffer &) = delete;

    FixedBuffer &operator=(const FixedBuffer &) = delete;

    void append(const char * /*restrict*/ buf, size_t len);

    const char *data() const
    { return data_; }

    int length() const
    { return static_cast<int>(cur_ - data_); }

    char *current()
    { return cur_; }

    int avail() const
    { return static_cast<int>(end() - cur_); }

    void add(size_t len)
    { cur_ += len; }

    void reset()
    { cur_ = data_; }

    void bzero()
    { memZero(data_, sizeof data_); }

    void set_cookie(std::function<void()> cookie)
    { cookie_ = std::move(cookie); }

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
    static void cookieStart();

    static void cookieEnd();

    std::function<void()> cookie_;
    char data_[SIZE];
    char *cur_;
};

}   // namespace detail

class LogStream
{
public:
    using self = LogStream;

    using Buffer = detail::FixedBuffer<detail::kSmallBuffer>;

    // overload stream opt <<
    self &operator<<(bool v);

    self &operator<<(short);

    self &operator<<(unsigned short);

    self &operator<<(int);

    self &operator<<(unsigned int);

    self &operator<<(long);

    self &operator<<(unsigned long);

    self &operator<<(long long);

    self &operator<<(unsigned long long);

private:
    template<typename T>
    void formatInteger(T);
private:
    Buffer buffer_;

    static const int k_max_num_size = 48;
};




}  // namespace wfrest

#endif //_LOGSTREAM_H_
