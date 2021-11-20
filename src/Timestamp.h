//
// Created by Chanchan on 11/19/21.
//

#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip> // put_time

using namespace std::chrono;

namespace wfrest
{
class Timestamp
{
public:
    Timestamp();

    Timestamp(const Timestamp &that);

    Timestamp &operator=(const Timestamp &that);

    // ms_since_epoch: The microseconds from 1970-01-01 00:00:00.
    explicit Timestamp(uint64_t ms_since_epoch);

    void swap(Timestamp &that);

    std::string to_str() const;

    std::string to_format_str() const;

    uint64_t ms_since_epoch() const;

    bool valid() const
    { return ms_since_epoch_ > 0; }

    static Timestamp now();

    static Timestamp invalid()
    { return Timestamp(); }

    static const int k_ms_per_sec = 1000 * 1000;
private:
    uint64_t ms_since_epoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.ms_since_epoch() < rhs.ms_since_epoch();
}

inline bool operator>(Timestamp lhs, Timestamp rhs)
{
    return lhs.ms_since_epoch() > rhs.ms_since_epoch();
}

inline bool operator<=(Timestamp lhs, Timestamp rhs)
{
    return lhs.ms_since_epoch() <= rhs.ms_since_epoch();
}

inline bool operator>=(Timestamp lhs, Timestamp rhs)
{
    return lhs.ms_since_epoch() >= rhs.ms_since_epoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.ms_since_epoch() == rhs.ms_since_epoch();
}

inline bool operator!=(Timestamp lhs, Timestamp rhs)
{
    return lhs.ms_since_epoch() != rhs.ms_since_epoch();
}

inline Timestamp operator+(Timestamp lhs, uint64_t ms)
{
    return Timestamp(lhs.ms_since_epoch() + ms);
}

inline Timestamp operator+(Timestamp lhs, double seconds)
{
    uint64_t delta = static_cast<uint64_t>(seconds * Timestamp::k_ms_per_sec);
    return Timestamp(lhs.ms_since_epoch() + delta);
}

inline Timestamp operator-(Timestamp lhs, uint64_t ms)
{
    return Timestamp(lhs.ms_since_epoch() - ms);
}

inline Timestamp operator-(Timestamp lhs, double seconds)
{
    uint64_t delta = static_cast<uint64_t>(seconds * Timestamp::k_ms_per_sec);
    return Timestamp(lhs.ms_since_epoch() - delta);
}

inline double operator-(Timestamp high, Timestamp low)
{
    uint64_t diff = high.ms_since_epoch() - low.ms_since_epoch();
    return static_cast<double>(diff) / Timestamp::k_ms_per_sec;
}

}  // namespace wfrest



#endif //_TIMESTAMP_H_
