## How to use logger

```cpp
#include "wfrest/Logger.h"
using namespace wfrest;

int main()
{
    // set the logger config
    LoggerSetting setting;
    setting.set_level(LogLevel::TRACE)
            .set_log_in_file(true);
    LOGGER(setting);

    int i = 1;
    LOG_DEBUG << (float)3.14;
    LOG_DEBUG << (const char)'8';
    LOG_DEBUG << &i;
    LOG_DEBUG << wfrest::Fmt("%.3f", 3.1415926);
    LOG_DEBUG << "debug";
    LOG_TRACE << "trace";
    LOG_INFO << "info";
    LOG_WARN << "warning";

    FILE *fp = fopen("/not_exist_file", "rb");
    if (fp == nullptr)
    {
        LOG_SYSERR << "syserr log!";
    }
    LOG_DEBUG  << 123 << 123.345 << "chanchan" << '\n'
               << std::string("name");
    return 0;
}
```

All the configure fields are:

```cpp
class LoggerSetting
{
    LogLevel level_ = LogLevel::INFO;
    bool log_in_console_ = true;
    bool log_in_file_ = false;
    std::string file_path_ = "./";
    std::string file_base_name_ = "wfrest";
    std::string file_extension_ = ".log";
    uint64_t roll_size_ = 20 * 1024 * 1024;
    std::chrono::seconds flush_interval_ = std::chrono::seconds(3);
};
```

Sample Output

INFO Level :

```
2021-12-14 11:30:56.006802 Here is the Info level
```

Other Level:

```
2021-11-30 22:36:21.422271 822380  [ERROR]  [logger_test.cc:84] No such file or directory (errno=2) syserr log
```

