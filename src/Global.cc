#include "Global.h"
#include "HttpFile.h"
#include "Logger.h"

using namespace wfrest;

// static member var
struct LoggerSettings Global::log_settings_ = LOGGER_SETTINGS_DEFAULT;

void WFREST_logger_init(const struct LoggerSettings *settings)
{
    Global::set_logger_settings(settings);
}
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


