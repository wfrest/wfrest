#ifndef WFREST_PUSHWRITEUTIL_H_
#define WFREST_PUSHWRITEUTIL_H_

#include <cerrno>
#include <climits>
#include <cstddef>

namespace wfrest
{
namespace detail
{

enum class PushWriteAction
{
    Complete,
    Retry,
    Fatal
};

struct PushWriteResult
{
    PushWriteAction action;
    size_t offset;
};

inline size_t push_write_size(size_t total_size, size_t offset)
{
    if (offset >= total_size)
        return 0;

    const size_t remaining = total_size - offset;
    const size_t max_write = static_cast<size_t>(INT_MAX);
    return remaining < max_write ? remaining : max_write;
}

inline PushWriteResult account_push_write(size_t total_size,
                                          size_t offset,
                                          int written,
                                          int error)
{
    if (offset > total_size)
        return {PushWriteAction::Fatal, offset};

    const size_t remaining = total_size - offset;
    if (written < 0)
    {
        if (error == EAGAIN || error == EWOULDBLOCK)
            return {PushWriteAction::Retry, offset};

        return {PushWriteAction::Fatal, offset};
    }

    if (written == 0)
    {
        const PushWriteAction action = remaining == 0
                                           ? PushWriteAction::Complete
                                           : PushWriteAction::Fatal;
        return {action, offset};
    }

    const size_t count = static_cast<size_t>(written);
    if (count > remaining)
        return {PushWriteAction::Fatal, offset};

    const size_t next_offset = offset + count;
    const PushWriteAction action = next_offset == total_size
                                       ? PushWriteAction::Complete
                                       : PushWriteAction::Retry;
    return {action, next_offset};
}

} // namespace detail
} // namespace wfrest

#endif // WFREST_PUSHWRITEUTIL_H_
