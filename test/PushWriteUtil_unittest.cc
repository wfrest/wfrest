#include <cerrno>
#include <climits>
#include <limits>
#include <gtest/gtest.h>
#include "../src/core/PushWriteUtil.h"

using wfrest::detail::PushWriteAction;
using wfrest::detail::PushWriteResult;
using wfrest::detail::account_push_write;
using wfrest::detail::push_write_size;

TEST(PushWriteUtil, completes_and_advances_partial_writes)
{
    PushWriteResult result = account_push_write(10, 0, 4, 0);
    EXPECT_EQ(result.action, PushWriteAction::Retry);
    EXPECT_EQ(result.offset, 4U);

    result = account_push_write(10, result.offset, 3, 0);
    EXPECT_EQ(result.action, PushWriteAction::Retry);
    EXPECT_EQ(result.offset, 7U);

    result = account_push_write(10, result.offset, 3, 0);
    EXPECT_EQ(result.action, PushWriteAction::Complete);
    EXPECT_EQ(result.offset, 10U);

    result = account_push_write(10, 10, 0, 0);
    EXPECT_EQ(result.action, PushWriteAction::Complete);
    EXPECT_EQ(result.offset, 10U);
}

TEST(PushWriteUtil, retries_would_block_without_advancing)
{
    PushWriteResult result = account_push_write(10, 4, -1, EAGAIN);
    EXPECT_EQ(result.action, PushWriteAction::Retry);
    EXPECT_EQ(result.offset, 4U);

    result = account_push_write(10, 4, -1, EWOULDBLOCK);
    EXPECT_EQ(result.action, PushWriteAction::Retry);
    EXPECT_EQ(result.offset, 4U);
}

TEST(PushWriteUtil, rejects_fatal_and_impossible_results)
{
    PushWriteResult result = account_push_write(10, 4, -1, EPIPE);
    EXPECT_EQ(result.action, PushWriteAction::Fatal);
    EXPECT_EQ(result.offset, 4U);

    result = account_push_write(10, 4, 0, 0);
    EXPECT_EQ(result.action, PushWriteAction::Fatal);
    EXPECT_EQ(result.offset, 4U);

    result = account_push_write(10, 4, 7, 0);
    EXPECT_EQ(result.action, PushWriteAction::Fatal);
    EXPECT_EQ(result.offset, 4U);

    result = account_push_write(10, 11, 1, 0);
    EXPECT_EQ(result.action, PushWriteAction::Fatal);
    EXPECT_EQ(result.offset, 11U);
}

TEST(PushWriteUtil, caps_calls_without_overflow)
{
    const size_t int_max = static_cast<size_t>(INT_MAX);
    EXPECT_EQ(push_write_size(int_max + 10, 0), int_max);
    EXPECT_EQ(push_write_size(int_max + 10, int_max), 10U);

    const size_t size_max = std::numeric_limits<size_t>::max();
    EXPECT_EQ(push_write_size(size_max, size_max - 5), 5U);
    EXPECT_EQ(push_write_size(size_max, size_max), 0U);
    EXPECT_EQ(push_write_size(10, 11), 0U);
}
