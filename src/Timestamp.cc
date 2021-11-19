//
// Created by Chanchan on 11/19/21.
//

#include "Timestamp.h"

using namespace wfrest;

static_assert(sizeof(Timestamp) == sizeof(uint64_t),
              "Timestamp should be same size as uint64_t");

Timestamp::Timestamp()
        : ms_since_epoch_(0)
{
}

Timestamp::Timestamp(const Timestamp &that)
        : ms_since_epoch_(that.ms_since_epoch_)
{
}

Timestamp::Timestamp(uint64_t microSecondsSinceEpoch)
        : ms_since_epoch_(microSecondsSinceEpoch)
{
}

Timestamp &Timestamp::operator=(const Timestamp &that)
{
    ms_since_epoch_ = that.ms_since_epoch_;
    return *this;
}

void Timestamp::swap(Timestamp &that)
{
    std::swap(ms_since_epoch_, that.ms_since_epoch_);
}

std::string Timestamp::toString() const
{
    return std::to_string(ms_since_epoch_ / k_ms_per_sec)
           + "." + std::to_string(ms_since_epoch_ % k_ms_per_sec);
}

std::string Timestamp::toFormatTime() const
{
    std::time_t time = ms_since_epoch_ / k_ms_per_sec;  // ms --> s
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %X");
    return ss.str();
}

uint64_t Timestamp::microSecondsSinceEpoch() const
{
    return ms_since_epoch_;
}

Timestamp Timestamp::now()
{
    uint64_t timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    return Timestamp(timestamp);
}