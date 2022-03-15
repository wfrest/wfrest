#include "Timestamp.h"

using namespace wfrest;

static_assert(sizeof(Timestamp) == sizeof(uint64_t),
              "Timestamp should be same size as uint64_t");

Timestamp::Timestamp()
        : micro_sec_since_epoch_(0)
{
}

Timestamp::Timestamp(uint64_t micro_sec_since_epoch)
        : micro_sec_since_epoch_(micro_sec_since_epoch)
{
}

Timestamp::Timestamp(const Timestamp &that)
        : micro_sec_since_epoch_(that.micro_sec_since_epoch_)
{
}

Timestamp &Timestamp::operator=(const Timestamp &that)
{
    micro_sec_since_epoch_ = that.micro_sec_since_epoch_;
    return *this;
}

void Timestamp::swap(Timestamp &that)
{
    std::swap(micro_sec_since_epoch_, that.micro_sec_since_epoch_);
}

std::string Timestamp::to_str() const
{
    return std::to_string(micro_sec_since_epoch_ / k_micro_sec_per_sec)
           + "." + std::to_string(micro_sec_since_epoch_ % k_micro_sec_per_sec);
}

std::string Timestamp::to_format_str() const
{
    return to_format_str("%Y-%m-%d %X");
}

std::string Timestamp::to_format_str(const char *fmt) const
{
    std::time_t time = micro_sec_since_epoch_ / k_micro_sec_per_sec;  // ms --> s
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), fmt);
    return ss.str();
}

uint64_t Timestamp::micro_sec_since_epoch() const
{
    return micro_sec_since_epoch_;
}

Timestamp Timestamp::now()
{
    uint64_t timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    return Timestamp(timestamp);
}

