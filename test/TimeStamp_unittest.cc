#include <gtest/gtest.h>

#include <atomic>
#include <cstdlib>
#include <ctime>
#include <locale>
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
