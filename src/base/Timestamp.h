#ifndef WFREST_TIMESTAMP_H_
#define WFREST_TIMESTAMP_H_

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip> // put_time

#include "Copyable.h"

using namespace std::chrono;

namespace wfrest
{
class Timestamp : public Copyable
{
public:
    Timestamp();

    Timestamp(const Timestamp &that);

    Timestamp &operator=(const Timestamp &that);

    // micro_sec_since_epoch: The microseconds from 1970-01-01 00:00:00.
    explicit Timestamp(uint64_t micro_sec_since_epoch);

    void swap(Timestamp &that);

    std::string to_str() const;

    std::string to_format_str() const;

    std::string to_format_str(const char *fmt) const;

    uint64_t micro_sec_since_epoch() const;

    bool valid() const
    { return micro_sec_since_epoch_ > 0; }

    static Timestamp now();

    static Timestamp invalid()
    { return Timestamp(); }

    static const int k_micro_sec_per_sec = 1000 * 1000;
private:
    uint64_t micro_sec_since_epoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.micro_sec_since_epoch() < rhs.micro_sec_since_epoch();
}

inline bool operator>(Timestamp lhs, Timestamp rhs)
{
    return lhs.micro_sec_since_epoch() > rhs.micro_sec_since_epoch();
}

inline bool operator<=(Timestamp lhs, Timestamp rhs)
{
    return lhs.micro_sec_since_epoch() <= rhs.micro_sec_since_epoch();
}

inline bool operator>=(Timestamp lhs, Timestamp rhs)
{
    return lhs.micro_sec_since_epoch() >= rhs.micro_sec_since_epoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.micro_sec_since_epoch() == rhs.micro_sec_since_epoch();
}

inline bool operator!=(Timestamp lhs, Timestamp rhs)
{
    return lhs.micro_sec_since_epoch() != rhs.micro_sec_since_epoch();
}

inline Timestamp operator+(Timestamp lhs, uint64_t ms)
{
    return Timestamp(lhs.micro_sec_since_epoch() + ms);
}

inline Timestamp operator+(Timestamp lhs, double seconds)
{
    uint64_t delta = static_cast<uint64_t>(seconds * Timestamp::k_micro_sec_per_sec);
    return Timestamp(lhs.micro_sec_since_epoch() + delta);
}

inline Timestamp operator-(Timestamp lhs, uint64_t ms)
{
    return Timestamp(lhs.micro_sec_since_epoch() - ms);
}

inline Timestamp operator-(Timestamp lhs, double seconds)
{
    uint64_t delta = static_cast<uint64_t>(seconds * Timestamp::k_micro_sec_per_sec);
    return Timestamp(lhs.micro_sec_since_epoch() - delta);
}

inline double operator-(Timestamp high, Timestamp low)
{
    uint64_t diff = high.micro_sec_since_epoch() - low.micro_sec_since_epoch();
    return static_cast<double>(diff) / Timestamp::k_micro_sec_per_sec;
}

}  // namespace wfrest

#endif // WFREST_TIMESTAMP_H_
