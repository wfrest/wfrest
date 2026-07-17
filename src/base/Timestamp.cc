#include "Timestamp.h"

#include <ctime>
#include <limits>
#include <locale>

using namespace wfrest;

namespace
{

enum class Timezone
{
    LOCAL,
    UTC
};

bool calendar_time(std::time_t value, Timezone timezone, std::tm *result)
{
#if defined(_WIN32)
    if (timezone == Timezone::UTC)
        return gmtime_s(result, &value) == 0;
    return localtime_s(result, &value) == 0;
#else
    if (timezone == Timezone::UTC)
        return gmtime_r(&value, result) != nullptr;
    return localtime_r(&value, result) != nullptr;
#endif
}

std::string format_timestamp(uint64_t microseconds,
                             const char *format,
                             Timezone timezone)
{
    if (format == nullptr)
        return {};

    const uint64_t seconds =
        microseconds / Timestamp::k_micro_sec_per_sec;
    if (seconds >
        static_cast<uint64_t>(std::numeric_limits<std::time_t>::max()))
    {
        return {};
    }

    const std::time_t value = static_cast<std::time_t>(seconds);
    std::tm calendar{};
    if (!calendar_time(value, timezone, &calendar))
        return {};

    std::stringstream stream;
    if (timezone == Timezone::UTC)
        stream.imbue(std::locale::classic());
    stream << std::put_time(&calendar, format);
    return stream.fail() ? std::string() : stream.str();
}

} // namespace

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
    return format_timestamp(micro_sec_since_epoch_, fmt, Timezone::LOCAL);
}

std::string Timestamp::to_utc_format_str() const
{
    return to_utc_format_str("%Y-%m-%d %X");
}

std::string Timestamp::to_utc_format_str(const char *fmt) const
{
    return format_timestamp(micro_sec_since_epoch_, fmt, Timezone::UTC);
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
