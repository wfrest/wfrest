#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>

#include "wfrest/SysInfo.h"

using namespace wfrest;

TEST(SysInfo, text_access_initializes_coherent_cache)
{
    const int length = CurrentThread::tid_str_len();
    const char *text = CurrentThread::tid_str();

    EXPECT_GT(length, 0);
    EXPECT_EQ(static_cast<size_t>(length), std::strlen(text));
    EXPECT_EQ(std::atoi(text), CurrentThread::tid());
    EXPECT_EQ(text[length - 1], ' ');
    EXPECT_EQ(CurrentThread::tid(), CurrentThread::gettid());
}

TEST(SysInfo, cache_is_initialized_independently_in_worker_thread)
{
    const int main_tid = CurrentThread::tid();
    int worker_tid = 0;
    int worker_actual = 0;
    int worker_length = 0;
    size_t worker_text_size = 0;

    std::thread worker([&]
    {
        worker_length = CurrentThread::tid_str_len();
        worker_text_size = std::strlen(CurrentThread::tid_str());
        worker_tid = CurrentThread::tid();
        worker_actual = CurrentThread::gettid();
    });
    worker.join();

    EXPECT_NE(worker_tid, main_tid);
    EXPECT_EQ(worker_tid, worker_actual);
    EXPECT_EQ(static_cast<size_t>(worker_length), worker_text_size);
    EXPECT_EQ(CurrentThread::tid(), main_tid);
}

TEST(SysInfo, child_refreshes_inherited_cache_after_fork)
{
    const int parent_tid = CurrentThread::tid();
    const pid_t child = fork();
    ASSERT_GE(child, 0);

    if (child == 0)
    {
        const int child_tid = CurrentThread::tid();
        const int child_actual = CurrentThread::gettid();
        const int length = CurrentThread::tid_str_len();
        const bool valid = child_tid == child_actual &&
                           child_tid != parent_tid &&
                           static_cast<size_t>(length) ==
                               std::strlen(CurrentThread::tid_str()) &&
                           std::atoi(CurrentThread::tid_str()) == child_tid;
        _exit(valid ? 0 : 1);
    }

    int status = 0;
    ASSERT_EQ(waitpid(child, &status, 0), child);
    ASSERT_TRUE(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 0);
    EXPECT_EQ(CurrentThread::tid(), parent_tid);
}
