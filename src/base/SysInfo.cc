#include <type_traits>
#include <pthread.h>
#include "SysInfo.h"

namespace wfrest
{

namespace CurrentThread
{
thread_local int t_cached_tid = 0;
thread_local char t_tid_str[32];
thread_local int t_tid_str_len = 0;
static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

namespace
{

pthread_once_t atfork_once = PTHREAD_ONCE_INIT;

void reset_tid_cache_after_fork()
{
    t_cached_tid = 0;
    t_tid_str[0] = '\0';
    t_tid_str_len = 0;
}

void install_tid_cache_atfork()
{
    (void)pthread_atfork(nullptr, nullptr, reset_tid_cache_after_fork);
}

} // namespace

namespace detail
{

void register_tid_cache_atfork()
{
    (void)pthread_once(&atfork_once, install_tid_cache_atfork);
}

} // namespace detail

}  // namespace CurrentThread
}  // namespace wfrest
