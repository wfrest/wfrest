#include "wfrest/Logger.h"

using namespace wfrest;

int main()
{
    LoggerSettings settings = LOGGER_SETTINGS_DEFAULT;
    settings.level = LogLevel::TRACE;
    settings.log_in_file = true;
    LOGGER(&settings);

    int i = 1;
    LOG_DEBUG << (float)3.14;
    LOG_DEBUG << (const char)'8';
    LOG_DEBUG << &i;
    LOG_DEBUG << wfrest::Fmt("%.3f", 3.1415926);
    LOG_DEBUG << "debug log!";
    LOG_TRACE << "trace log!";
    LOG_INFO << "info log!";
    LOG_WARN << "warning log!";

    FILE *fp = fopen("/not_exist_file", "rb");
    if (fp == nullptr)
    {
        LOG_SYSERR << "syserr log!" << 7;
    }
    LOG_DEBUG  << 123 << 123.123 << "haha" << '\n'
               << std::string("12356");
    return 0;
}