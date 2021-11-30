#ifndef WFREST_GLOBAL_H_
#define WFREST_GLOBAL_H_

#include "Logger.h"
#include "AsyncFileLogger.h"

namespace wfrest
{
class HttpFile;

class Global
{
public:
    static HttpFile *get_http_file();

    static LoggerSettings *get_logger_settings();

    static void set_logger_settings(const struct LoggerSettings *log_settings);

    static AsyncFileLogger *get_async_file_logger();
private:
    static struct LoggerSettings log_settings_;
};

}  // namespace wfrest


#endif // WFREST_GLOBAL_H_
