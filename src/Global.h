#ifndef c_GLOBAL_H_
#define WFREST_GLOBAL_H_

namespace wfrest
{
class HttpFile;

struct LoggerSettings;

void WFREST_logger_init(const struct LoggerSettings *settings);

class Global
{
public:
    static HttpFile *get_http_file();

    static LoggerSettings *get_logger_settings();

    static void set_logger_settings(const struct LoggerSettings *log_settings);
private:
    static struct LoggerSettings log_settings_;
};

}  // namespace wfrest


#endif // WFREST_GLOBAL_H_
