#include <type_traits>
#include "SysInfo.h"

namespace wfrest
{

namespace CurrentThread
{
thread_local int t_cached_tid = 0;
thread_local char t_tid_str[32];
thread_local int t_tid_str_len = 6;
static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

}  // namespace CurrentThread
}  // namespace wfrest
