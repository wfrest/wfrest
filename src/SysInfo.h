//
// Created by Chanchan on 11/20/21.
//

#ifndef _SYSINFO_H_
#define _SYSINFO_H_

#include <ctime>
#include <unistd.h>
#include <sys/syscall.h>
#include <cstdio>

namespace wfrest
{

namespace CurrentThread
{
// internal
extern thread_local int t_cached_tid;
extern thread_local char t_tid_str[32];
extern thread_local int t_tid_str_len;

static inline pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

static inline void cacheTid()
{
    if (t_cached_tid == 0)
    {
        t_cached_tid = gettid();
        t_tid_str_len = snprintf(t_tid_str, sizeof t_tid_str, "%5d ", t_cached_tid);
    }
}

static inline int tid()
{
    if (__builtin_expect(t_cached_tid == 0, 0))
    {
        cacheTid();
    }
    return t_cached_tid;
}

// for logging
static inline const char *tid_str() { return t_tid_str; }
// for logging
static inline int tid_str_len() { return t_tid_str_len; }

}  // namespace CurrentThread

}  // wfrest


#endif //_SYSINFO_H_
