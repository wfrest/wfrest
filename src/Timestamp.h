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
    class Timestamp {
    public:
        Timestamp();
        Timestamp(const Timestamp& that);
        Timestamp& operator=(const Timestamp& that);
        explicit Timestamp(uint64_t microSecondsSinceEpoch);

        void swap(Timestamp& that);
        std::string toString() const;
        std::string toFormatTime() const;
        uint64_t microSecondsSinceEpoch() const;
        bool valid() const { return ms_since_epoch_ > 0; }

        static Timestamp now();
        static Timestamp invalid() { return Timestamp(); }
        static const int k_ms_per_sec = 1000 * 1000;
    private:
        uint64_t ms_since_epoch_;
    };

    inline bool operator<(Timestamp lhs, Timestamp rhs) {
        return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
    }

    inline bool operator>(Timestamp lhs, Timestamp rhs) {
        return lhs.microSecondsSinceEpoch() > rhs.microSecondsSinceEpoch();
    }

    inline bool operator<=(Timestamp lhs, Timestamp rhs) {
        return lhs.microSecondsSinceEpoch() <= rhs.microSecondsSinceEpoch();
    }

    inline bool operator>=(Timestamp lhs, Timestamp rhs) {
        return lhs.microSecondsSinceEpoch() >= rhs.microSecondsSinceEpoch();
    }

    inline bool operator==(Timestamp lhs, Timestamp rhs) {
        return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
    }

    inline bool operator!=(Timestamp lhs, Timestamp rhs) {
        return lhs.microSecondsSinceEpoch() != rhs.microSecondsSinceEpoch();
    }

    inline Timestamp operator+(Timestamp lhs, uint64_t ms) {
        return Timestamp(lhs.microSecondsSinceEpoch() + ms);
    }

    inline Timestamp operator+(Timestamp lhs, double seconds) {
        uint64_t delta = static_cast<uint64_t>(seconds * Timestamp::k_ms_per_sec);
        return Timestamp(lhs.microSecondsSinceEpoch() + delta);
    }

    inline Timestamp operator-(Timestamp lhs, uint64_t ms) {
        return Timestamp(lhs.microSecondsSinceEpoch() - ms);
    }

    inline Timestamp operator-(Timestamp lhs, double seconds) {
        uint64_t delta = static_cast<uint64_t>(seconds * Timestamp::k_ms_per_sec);
        return Timestamp(lhs.microSecondsSinceEpoch() - delta);
    }

    inline double operator-(Timestamp high, Timestamp low) {
        uint64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
        return static_cast<double>(diff) / Timestamp::k_ms_per_sec;
    }

}  // namespace wfrest



#endif //_TIMESTAMP_H_
