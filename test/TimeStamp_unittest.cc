#include <gtest/gtest.h>

#include <atomic>
#include <cstdlib>
#include <ctime>
#include <locale>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#include "wfrest/Timestamp.h"

using namespace wfrest;

namespace
{

class ScopedTimezone
{
public:
    explicit ScopedTimezone(const char *timezone)
    {
        const char *current = std::getenv("TZ");
        if (current != nullptr)
        {
            had_value_ = true;
            value_ = current;
        }

        (void)setenv("TZ", timezone, 1);
        tzset();
    }

    ~ScopedTimezone()
    {
        if (had_value_)
            (void)setenv("TZ", value_.c_str(), 1);
        else
            (void)unsetenv("TZ");
        tzset();
    }

private:
    bool had_value_ = false;
    std::string value_;
};

class MarkerTimePut : public std::time_put<char>
{
protected:
    iter_type do_put(iter_type output,
                     std::ios_base&,
                     char_type,
                     const std::tm *,
                     char,
                     char) const override
    {
        *output++ = 'X';
        return output;
    }
};

class ScopedMarkerLocale
{
public:
    ScopedMarkerLocale()
        : previous_(std::locale())
    {
        std::locale::global(std::locale(previous_, new MarkerTimePut));
    }

    ~ScopedMarkerLocale()
    {
        std::locale::global(previous_);
    }

private:
    std::locale previous_;
};

} // namespace

TEST(Timestamp, serializes_exact_microsecond_fraction)
{
    EXPECT_EQ(Timestamp(0).to_str(), "0.000000");
    EXPECT_EQ(Timestamp(1).to_str(), "0.000001");
    EXPECT_EQ(Timestamp(1000001).to_str(), "1.000001");
    EXPECT_EQ(Timestamp(1100000).to_str(), "1.100000");
}

TEST(Timestamp, saturates_integer_timepoint_arithmetic)
{
    const uint64_t maximum = std::numeric_limits<uint64_t>::max();

    EXPECT_EQ((Timestamp(10) + static_cast<uint64_t>(5))
                  .micro_sec_since_epoch(), 15U);
    EXPECT_EQ((Timestamp(maximum - 1) + static_cast<uint64_t>(1))
                  .micro_sec_since_epoch(), maximum);
    EXPECT_EQ((Timestamp(maximum - 1) + static_cast<uint64_t>(2))
                  .micro_sec_since_epoch(), maximum);

    EXPECT_EQ((Timestamp(10) - static_cast<uint64_t>(5))
                  .micro_sec_since_epoch(), 5U);
    EXPECT_EQ((Timestamp(5) - static_cast<uint64_t>(5))
                  .micro_sec_since_epoch(), 0U);
    EXPECT_EQ((Timestamp(5) - static_cast<uint64_t>(6))
                  .micro_sec_since_epoch(), 0U);
}

TEST(Timestamp, applies_floating_seconds_without_invalid_casts)
{
    const Timestamp base(2000000);
    EXPECT_EQ((base + 1.5).micro_sec_since_epoch(), 3500000U);
    EXPECT_EQ((base - 1.5).micro_sec_since_epoch(), 500000U);
    EXPECT_EQ((base + -1.5).micro_sec_since_epoch(), 500000U);
    EXPECT_EQ((base - -1.5).micro_sec_since_epoch(), 3500000U);
    EXPECT_EQ((base + 0.0000009).micro_sec_since_epoch(), 2000000U);
    EXPECT_EQ((base + 0.0000019).micro_sec_since_epoch(), 2000001U);

    const double infinity = std::numeric_limits<double>::infinity();
    const double maximum = std::numeric_limits<double>::max();
    const uint64_t max_timestamp = std::numeric_limits<uint64_t>::max();
    EXPECT_EQ((base + infinity).micro_sec_since_epoch(), max_timestamp);
    EXPECT_EQ((base + -infinity).micro_sec_since_epoch(), 0U);
    EXPECT_EQ((base - infinity).micro_sec_since_epoch(), 0U);
    EXPECT_EQ((base - -infinity).micro_sec_since_epoch(), max_timestamp);
    EXPECT_EQ((base + maximum).micro_sec_since_epoch(), max_timestamp);
    EXPECT_EQ((base + -maximum).micro_sec_since_epoch(), 0U);

    const double nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ((base + nan).micro_sec_since_epoch(),
              base.micro_sec_since_epoch());
    EXPECT_EQ((base - nan).micro_sec_since_epoch(),
              base.micro_sec_since_epoch());
}

TEST(Timestamp, computes_signed_duration_differences)
{
    const Timestamp early(1000000);
    const Timestamp late(2000000);
    EXPECT_DOUBLE_EQ(late - early, 1.0);
    EXPECT_DOUBLE_EQ(early - late, -1.0);
    EXPECT_DOUBLE_EQ(early - early, 0.0);

    const Timestamp maximum(std::numeric_limits<uint64_t>::max());
    const double forward = maximum - Timestamp();
    const double reverse = Timestamp() - maximum;
    EXPECT_GT(forward, 0.0);
    EXPECT_LT(reverse, 0.0);
    EXPECT_DOUBLE_EQ(forward, -reverse);
}

TEST(Timestamp, separates_local_and_utc_formatting)
{
    ScopedTimezone timezone("EST5");
    const Timestamp instant(1000000);

    EXPECT_EQ(instant.to_format_str("%Y-%m-%d %H:%M:%S"),
              "1969-12-31 19:00:01");
    EXPECT_EQ(instant.to_utc_format_str("%Y-%m-%d %H:%M:%S"),
              "1970-01-01 00:00:01");
    EXPECT_EQ(instant.to_utc_format_str("%a, %d %b %Y %H:%M:%S GMT"),
              "Thu, 01 Jan 1970 00:00:01 GMT");
    EXPECT_FALSE(instant.to_format_str().empty());
    EXPECT_FALSE(instant.to_utc_format_str().empty());
}

TEST(Timestamp, rejects_null_formats)
{
    const Timestamp instant(1000000);
    EXPECT_TRUE(instant.to_format_str(nullptr).empty());
    EXPECT_TRUE(instant.to_utc_format_str(nullptr).empty());
    EXPECT_TRUE(instant.to_format_str("").empty());
    EXPECT_TRUE(instant.to_utc_format_str("").empty());
}

TEST(Timestamp, utc_formatting_uses_the_classic_locale)
{
    ScopedTimezone timezone("UTC0");
    ScopedMarkerLocale locale;
    const Timestamp instant(1000000);

    EXPECT_EQ(instant.to_format_str("%a"), "X");
    EXPECT_EQ(instant.to_utc_format_str("%a"), "Thu");
}

TEST(Timestamp, formats_concurrently_from_owned_calendar_storage)
{
    ScopedTimezone timezone("EST5");
    constexpr size_t thread_count = 12;
    constexpr size_t iterations = 1000;
    const char *format = "%Y-%m-%d %H:%M:%S";

    std::vector<Timestamp> timestamps;
    std::vector<std::string> expected_local;
    std::vector<std::string> expected_utc;
    for (size_t i = 0; i < thread_count; ++i)
    {
        timestamps.emplace_back(
            (1 + i * 24 * 60 * 60) * Timestamp::k_micro_sec_per_sec);
        expected_local.emplace_back(timestamps.back().to_format_str(format));
        expected_utc.emplace_back(
            timestamps.back().to_utc_format_str(format));
    }

    std::atomic<bool> valid(true);
    std::vector<std::thread> threads;
    for (size_t i = 0; i < thread_count; ++i)
    {
        threads.emplace_back([&, i]
        {
            for (size_t j = 0; j < iterations; ++j)
            {
                if (timestamps[i].to_format_str(format) != expected_local[i] ||
                    timestamps[i].to_utc_format_str(format) != expected_utc[i])
                {
                    valid.store(false, std::memory_order_relaxed);
                    return;
                }
            }
        });
    }

    for (std::thread& thread : threads)
        thread.join();
    EXPECT_TRUE(valid.load(std::memory_order_relaxed));
}
