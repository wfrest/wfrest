// Taken from PCRE pcre_stringpiece.h
//
// Copyright (c) 2005, Google Inc.
// All rights reserved.

#ifndef WFREST_STRINGPIECE_H_
#define WFREST_STRINGPIECE_H_

#include <cstring>
#include <string>

namespace wfrest
{
class StringPiece
{
private:
    const char *ptr_;
    size_t length_;

public:
    // We provide non-explicit singleton constructors so users can pass
    // in a "const char*" or a "std::string" wherever a "StringPiece" is
    // expected.
    StringPiece()
            : ptr_(nullptr), length_(0)
    {}

    StringPiece(const char *str)
            : ptr_(str), length_(strlen(ptr_))
    {}

    StringPiece(const std::string &str)
            : ptr_(str.data()), length_(str.size())
    {}

    StringPiece(const char *offset, size_t len)
            : ptr_(offset), length_(len)
    {}

    StringPiece(const void *str, size_t len)
            : ptr_(static_cast<const char *>(str)), length_(len)
    {}

    // data() may return a pointer to a buffer with embedded NULs, and the
    // returned buffer may or may not be null terminated.  Therefore it is
    // typically a mistake to pass data() to a routine that expects a NUL
    // terminated std::string.  Use "as_string().c_str()" if you really need to do
    // this.  Or better yet, change your routine so it does not rely on NUL
    // termination.
    const char *data() const
    { return ptr_; }

    size_t size() const
    { return length_; }

    bool empty() const
    { return length_ == 0; }

    const char *begin() const
    { return ptr_; }

    const char *end() const
    { return ptr_ + length_; }

    void clear()
    {
        ptr_ = nullptr;
        length_ = 0;
    }

    void set(const char *buffer, int len)
    {
        ptr_ = buffer;
        length_ = len;
    }

    void set(const char *str)
    {
        ptr_ = str;
        length_ = strlen(str);
    }

    void set(const char *buffer, size_t len)
    {
        ptr_ = buffer;
        length_ = len;
    }

    void set(const void *buffer, size_t len)
    {
        ptr_ = static_cast<const char *>(buffer);
        length_ = len;
    }

    char operator[](int i) const
    { return ptr_[i]; }

    void remove_prefix(size_t n)
    {
        ptr_ += n;
        length_ -= n;
    }

    void remove_suffix(size_t n)
    {
        length_ -= n;
    }

    void shrink(size_t prefix, size_t suffix)
    {
        ptr_ += prefix;
        length_ -= prefix;
        length_ -= suffix;
    }

    bool operator==(const StringPiece &x) const
    {
        return ((length_ == x.length_) &&
                (memcmp(ptr_, x.ptr_, length_) == 0));
    }

    bool operator!=(const StringPiece &x) const
    {
        return !(*this == x);
    }

#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp)                             \
    bool operator cmp (const StringPiece& x) const {                           \
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); \
        return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));          \
    }

    STRINGPIECE_BINARY_PREDICATE(<, <);

    STRINGPIECE_BINARY_PREDICATE(<=, <);

    STRINGPIECE_BINARY_PREDICATE(>=, >);

    STRINGPIECE_BINARY_PREDICATE(>, >);
#undef STRINGPIECE_BINARY_PREDICATE

    int compare(const StringPiece &x) const
    {
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
        if (r == 0)
        {
            if (length_ < x.length_) r = -1;
            else if (length_ > x.length_) r = +1;
        }
        return r;
    }

    std::string as_string() const
    {
        return std::string(data(), size());
    }
//
//        std::string to_string() const
//        {
//            return std::string(data(), size() + 1);
//        }

    void CopyToString(std::string *target) const
    {
        target->assign(ptr_, length_);
    }

    // Does "this" start with "x"
    bool starts_with(const StringPiece &x) const
    {
        return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
    }
};

}  // namespace wfrest

// ------------------------------------------------------------------
// Functions used to create STL containers that use StringPiece
//  Remember that a StringPiece's lifetime had better be less than
//  that of the underlying std::string or char*.  If it is not, then you
//  cannot safely store a StringPiece into an STL container
// ------------------------------------------------------------------

#ifdef HAVE_TYPE_TRAITS
// This makes vector<StringPiece> really fast for some STL implementations
template<> struct __type_traits<wfrest::StringPiece> {
    typedef __true_type    has_trivial_default_constructor;
    typedef __true_type    has_trivial_copy_constructor;
    typedef __true_type    has_trivial_assignment_operator;
    typedef __true_type    has_trivial_destructor;
    typedef __true_type    is_POD_type;
};
#endif

// Stand-ins for the STL's std::hash<> specializations.
template<typename StringPieceType>
struct StringPieceHashImpl
{
    // This is a custom hash function. We don't use the ones already defined for
    // string and std::u16string directly because it would require the string
    // constructors to be called, which we don't want.
    std::size_t operator()(StringPieceType sp) const
    {
        std::size_t result = 0;
        for (auto c: sp)
            result = (result * 131) + c;
        return result;
    }
};

using StringPieceHash = StringPieceHashImpl<wfrest::StringPiece>;

#endif // WFREST_STRINGPIECE_H_