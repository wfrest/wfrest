#include "Logger.h"
#include <cerrno>
#include <thread>

using namespace wfrest;

// Basic use of logger
void test01()
{
    LOG_TRACE << "trace ...";
    LOG_DEBUG << "debug ...";
    LOG_INFO << "info ...";
    LOG_WARN << "warn ...";
    LOG_ERROR << "error ...";
    //LOG_FATAL<<"fatal ...";
    errno = 13;
    LOG_SYSERR << "syserr ...";
    //LOG_SYSFATAL<<"sysfatal ...";
}

// how to set the setting
void test02()
{
    LoggerSettings settings = LOGGER_SETTINGS_DEFAULT;
    settings.level = LogLevel::TRACE;
    settings.log_in_file = true;
    LOGGER(&settings);

    LOG_TRACE << "trace ...";
    LOG_DEBUG << "debug ...";
    LOG_INFO << "info ...";
    LOG_WARN << "warn ...";
    LOG_ERROR << "error ...";
    //LOG_FATAL<<"fatal ...";
    errno = 13;
    LOG_SYSERR << "syserr ...";
    //LOG_SYSFATAL<<"sysfatal ...";
}

// some other types
void test03()
{
    LoggerSettings settings = LOGGER_SETTINGS_DEFAULT;
    settings.level = LogLevel::TRACE;
    LOGGER(&settings);

    int i = 1;
    LOG_DEBUG << (float)3.14;
    LOG_DEBUG << (const char)'8';
    LOG_DEBUG << &i;
    LOG_DEBUG << wfrest::Fmt("%.3f", 3.1415926);
    LOG_DEBUG << "debug log!" << 1;
    LOG_TRACE << "trace log!" << 2;
    LOG_INFO << "info log!" << 3;
    LOG_WARN << "warning log!" << 4;

    FILE *fp = fopen("/not_exist_file", "rb");
    if (fp == nullptr)
    {
        LOG_SYSERR << "syserr log!" << 7;
    }
    LOG_DEBUG  << 123 << 123.123 << "haha" << '\n'
               << std::string("12356");
}

// multi thread
void test04()
{
    int i = 1;
    LOG_DEBUG << (float)3.14;
    LOG_DEBUG << (const char)'8';
    LOG_DEBUG << &i;
    LOG_DEBUG << wfrest::Fmt("%.3f", 3.1415926);
    LOG_DEBUG << "debug log!" << 1;
    LOG_TRACE << "trace log!" << 2;
    LOG_INFO << "info log!" << 3;
    LOG_WARN << "warning log!" << 4;

    std::thread thread([]() { LOG_FATAL << "fatal log!" << 6; });

    FILE *fp = fopen("/not_exist_file", "rb");
    if (fp == nullptr)
    {
        LOG_SYSERR << "syserr log!" << 7;
    }
    LOG_DEBUG  << 123 << 123.123 << "haha" << '\n'
               << std::string("12356");

    thread.join();
}
int main()
{
    // test01();
    // test02();
    // test03();
    test04();
}