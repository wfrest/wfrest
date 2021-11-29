#include "Global.h"
#include "HttpFile.h"

using namespace wfrest;

// static member var
struct LoggerSettings Global::log_settings_ = LOGGER_SETTINGS_DEFAULT;

HttpFile *Global::get_http_file()
{
    return HttpFile::get_instance();
}

LoggerSettings *Global::get_logger_settings()
{
    return &log_settings_;
}

void Global::set_logger_settings(const struct LoggerSettings *log_settings)
{
    log_settings_ = *log_settings;
}

AsyncFileLogger *Global::get_async_file_logger()
{
    static AsyncFileLogger async_file_logger;
    return &async_file_logger;
}