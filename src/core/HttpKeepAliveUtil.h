#ifndef WFREST_HTTPKEEPALIVEUTIL_H_
#define WFREST_HTTPKEEPALIVEUTIL_H_

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

namespace wfrest
{
namespace detail
{

constexpr int k_keep_alive_timeout_max_ms = 300 * 1000;

inline bool is_keep_alive_ows(char ch)
{
    return ch == ' ' || ch == '\t';
}

inline void trim_keep_alive_ows(const std::string &value,
                                size_t *begin,
                                size_t *end)
{
    while (*begin < *end && is_keep_alive_ows(value[*begin]))
        ++*begin;
    while (*end > *begin && is_keep_alive_ows(value[*end - 1]))
        --*end;
}

inline unsigned char keep_alive_ascii_lower(unsigned char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return static_cast<unsigned char>(ch + ('a' - 'A'));
    return ch;
}

inline bool keep_alive_key_equals(const std::string &value,
                                  size_t begin,
                                  size_t end,
                                  const char *expected)
{
    size_t cursor = begin;
    size_t expected_cursor = 0;
    while (cursor < end && expected[expected_cursor] != '\0')
    {
        if (keep_alive_ascii_lower(
                static_cast<unsigned char>(value[cursor])) !=
            keep_alive_ascii_lower(
                static_cast<unsigned char>(expected[expected_cursor])))
        {
            return false;
        }
        ++cursor;
        ++expected_cursor;
    }

    return cursor == end && expected[expected_cursor] == '\0';
}

inline bool parse_keep_alive_decimal(const std::string &value,
                                     size_t begin,
                                     size_t end,
                                     uint64_t limit,
                                     uint64_t *result)
{
    if (begin == end)
        return false;

    uint64_t parsed = 0;
    for (size_t cursor = begin; cursor < end; ++cursor)
    {
        const unsigned char ch =
            static_cast<unsigned char>(value[cursor]);
        if (ch < '0' || ch > '9')
            return false;

        const uint64_t digit = static_cast<uint64_t>(ch - '0');
        if (parsed > limit / 10 ||
            (parsed == limit / 10 && digit > limit % 10))
        {
            parsed = limit;
        }
        else
        {
            parsed = parsed * 10 + digit;
        }
    }

    *result = parsed;
    return true;
}

inline int resolve_keep_alive_timeout(const std::string &header_value,
                                      long long connection_sequence,
                                      int configured_timeout_ms)
{
    int timeout_ms = configured_timeout_ms;
    if (timeout_ms < 0 || timeout_ms > k_keep_alive_timeout_max_ms)
        timeout_ms = k_keep_alive_timeout_max_ms;

    bool has_timeout = false;
    bool has_max = false;
    uint64_t max_requests = 0;

    size_t segment_begin = 0;
    while (segment_begin <= header_value.size())
    {
        size_t segment_end = header_value.find(',', segment_begin);
        if (segment_end == std::string::npos)
            segment_end = header_value.size();

        size_t begin = segment_begin;
        size_t end = segment_end;
        trim_keep_alive_ows(header_value, &begin, &end);

        const size_t equals = header_value.find('=', begin);
        if (equals < end)
        {
            size_t key_begin = begin;
            size_t key_end = equals;
            trim_keep_alive_ows(header_value, &key_begin, &key_end);

            size_t value_begin = equals + 1;
            size_t value_end = end;
            trim_keep_alive_ows(header_value, &value_begin, &value_end);

            if (!has_timeout && keep_alive_key_equals(
                    header_value, key_begin, key_end, "timeout"))
            {
                uint64_t seconds;
                if (parse_keep_alive_decimal(header_value,
                                             value_begin,
                                             value_end,
                                             300,
                                             &seconds))
                {
                    has_timeout = true;
                    const int requested_ms =
                        static_cast<int>(seconds * 1000);
                    timeout_ms = std::min(timeout_ms, requested_ms);
                }
            }
            else if (!has_max && keep_alive_key_equals(
                         header_value, key_begin, key_end, "max"))
            {
                uint64_t parsed_max;
                if (parse_keep_alive_decimal(
                        header_value,
                        value_begin,
                        value_end,
                        std::numeric_limits<uint64_t>::max(),
                        &parsed_max))
                {
                    has_max = true;
                    max_requests = parsed_max;
                }
            }
        }

        if (segment_end == header_value.size())
            break;
        segment_begin = segment_end + 1;
    }

    if (has_max)
    {
        uint64_t request_ordinal = 1;
        if (connection_sequence >= 0)
        {
            request_ordinal =
                static_cast<uint64_t>(connection_sequence) + 1;
        }

        if (request_ordinal >= max_requests)
            timeout_ms = 0;
    }

    return timeout_ms;
}

} // namespace detail
} // namespace wfrest

#endif // WFREST_HTTPKEEPALIVEUTIL_H_
