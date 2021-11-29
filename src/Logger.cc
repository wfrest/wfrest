#include "Logger.h"
#include "SysInfo.h"
#include "Global.h"

namespace wfrest
{

// for optimizing time buf
thread_local char t_errnobuf[512];
thread_local time_t t_last_sec;
thread_local char t_time[64];

const char *strerror_tl(int savedErrno)
{
    return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

const char* log_level_to_str(const LogLevel& level)
{
    switch (level)
    {
        case LogLevel::TRACE:
            return " [TRACE] ";
        case LogLevel::DEBUG:
            return " [DEBUG] ";
        case LogLevel::INFO:
            return " [INFO]  ";
        case LogLevel::WARN:
            return " [WARN]  ";
        case LogLevel::ERROR:
            return " [ERROR] ";
        case LogLevel::FATAL:
            return " [FATAL] ";
        default:
            return "[UNKNOWN]";
    }
}

// helper class for known string length at compile time
class T
{
public:
    T(const char *str, unsigned len)
            : str_(str),
              len_(len)
    {
        assert(strlen(str) == len_);
    }

public:
    const char *str_;
    const unsigned len_;
};

inline LogStream &operator<<(LogStream &s, T v)
{
    s.append(v.str_, v.len_);
    return s;
}

inline LogStream &operator<<(LogStream &s, const Logger::SourceFile &v)
{
    s.append(v.data_, v.size_);
    return s;
}

void WFREST_init_logger(struct LoggerSettings *settings)
{
    Global::set_logger_settings(settings);
    if(settings->log_in_file)
        Global::get_async_file_logger()->start();
}

}  // namespace wfrest

using namespace wfrest;

Logger::SourceFile::SourceFile(const char *filename)
        : data_(filename)
{
    const char *slash = strrchr(filename, '/');
    if (slash)
    {
        data_ = slash + 1;
    }
    size_ = static_cast<int>(strlen(data_));
}

Logger::Logger(Logger::SourceFile file, int line)
        : impl_(LogLevel::INFO, 0, file, line)
{
}

Logger::Logger(Logger::SourceFile file, int line, LogLevel level)
        : impl_(level, 0, file, line)
{
}

Logger::Logger(Logger::SourceFile file, int line, LogLevel level, const char *func)
        : impl_(level, 0, file, line)
{
    impl_.stream_ << "[" << func << "] ";
}

Logger::Logger(Logger::SourceFile file, int line, bool toAbort)
        : impl_(toAbort ? LogLevel::FATAL : LogLevel::ERROR, errno, file, line)
{
}

Logger::~Logger()
{
    impl_.stream_ << "\n";
    const LogStream::Buffer &buf(stream().buffer());
    // Call g_output to output the log data (stored in buf).
    output_func_()(buf.data(), buf.length());
    if (impl_.level_ == LogLevel::FATAL)
    {
        flush_func_()();
        abort();
    }
}

Logger::Impl::Impl(LogLevel level, int savedErrno, const Logger::SourceFile &file, int line)
        : time_(Timestamp::now()),
          stream_(),
          level_(level),
          line_(line),
          basename_(file)
{
    formatTime();
    CurrentThread::tid();
    stream_ << T(CurrentThread::tid_str(), CurrentThread::tid_str_len());
    stream_ << T(log_level_to_str(level), 9);
    stream_ << " [" << basename_ << ':' << line_ << "] ";
    if (savedErrno != 0)
    {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

void Logger::Impl::formatTime()
{
    int64_t ms_since_epoch = time_.micro_sec_since_epoch();
    time_t sec = static_cast<time_t>(ms_since_epoch / Timestamp::k_micro_sec_per_sec);
    int ms = static_cast<int>(ms_since_epoch % Timestamp::k_micro_sec_per_sec);
    if (sec != t_last_sec)
    {
        t_last_sec = sec;
        struct tm tm_time;
        ::gmtime_r(&sec, &tm_time);

        int len = snprintf(t_time, sizeof(t_time), "%4d-%02d-%02d %02d:%02d:%02d",
                           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 19);
    }

    Fmt us(".%06d ", ms);
    assert(us.length() == 8);
    stream_ << T(t_time, 19) << T(us.data(), 8);
}

void Logger::default_output(const char *msg, int len)
{
    size_t n = fwrite(msg, 1, len, stderr);
}

void Logger::file_output(const char *msg, int len)
{
    Global::get_async_file_logger()->output(msg, len);
}

void Logger::default_flush()
{
    fflush(stderr);
}

Logger::OutputFunc &Logger::output_func_()
{
    static OutputFunc output_func =
            Global::get_logger_settings()->log_in_file ?
            Logger::file_output : Logger::default_output;
    return output_func;
}

Logger::FlushFunc &Logger::flush_func_()
{
    static FlushFunc flush_func = Logger::default_flush;
    return flush_func;
}

void Logger::set_output(const Logger::OutputFunc &output_func, const Logger::FlushFunc &flush_func)
{
    output_func_() = output_func;
    flush_func_() = flush_func;
}

void Logger::set_output(Logger::OutputFunc &&output_func, Logger::FlushFunc &&flush_func)
{
    output_func_() = std::move(output_func);
    flush_func_() = std::move(flush_func);
}

LogLevel Logger::log_level()
{
    return Global::get_logger_settings()->level;
}







