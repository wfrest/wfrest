//
// Created by Chanchan on 11/18/21.
//

#include "Logger.h"
#include "SysInfo.h"

namespace wfrest
{

const char *log_level_str[Logger::NUM_LOG_LEVELS] =
        {
                " TRACE ",
                " DEBUG ",
                " INFO  ",
                " WARN  ",
                " ERROR ",
                " FATAL ",
        };

thread_local char t_errnobuf[512];
thread_local time_t t_last_sec;
thread_local char t_time[64];

const char *strerror_tl(int savedErrno)
{
    return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
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

}  // namespace wfrest

using namespace wfrest;

template<int N>
Logger::SourceFile::SourceFile(const char (&arr)[N])
        : data_(arr),
          size_(N - 1)
{
    const char *slash = strrchr(data_, '/'); // builtin function
    if (slash)
    {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
    }
}

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
        : impl_(INFO, 0, file, line)
{
}

Logger::Logger(Logger::SourceFile file, int line, Logger::LogLevel level)
        : impl_(level, 0, file, line)
{
}

Logger::Logger(Logger::SourceFile file, int line, Logger::LogLevel level, const char *func)
        : impl_(level, 0, file, line)
{
    impl_.stream_ << "[" << func << "] ";
}

Logger::Logger(Logger::SourceFile file, int line, bool toAbort)
        : impl_(toAbort ? FATAL : ERROR, errno, file, line)
{
}

Logger::~Logger()
{
    impl_.finish();  // only add file name and line num in buffer
    const LogStream::Buffer &buf(stream().buffer());
    // Call g_output to output the log data (stored in buf).
    output_func_()(buf.data(), buf.length());
    if (impl_.level_ == FATAL)
    {
        flush_func_()();
        abort();
    }
}

void Logger::Impl::finish()
{
    stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Impl::Impl(Logger::LogLevel level, int savedErrno, const Logger::SourceFile &file, int line)
        : time_(Timestamp::now()),
          stream_(),
          level_(level),
          line_(line),
          basename_(file)
{
    formatTime();
    CurrentThread::tid();
    stream_ << T(CurrentThread::tid_str(), CurrentThread::tid_str_len());
    stream_ << T(log_level_str[level], 7);
    if (savedErrno != 0)
    {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

void Logger::Impl::formatTime()
{
    int64_t ms_since_epoch = time_.ms_since_epoch();
    time_t sec = static_cast<time_t>(ms_since_epoch / Timestamp::k_ms_per_sec);
    int ms = static_cast<int>(ms_since_epoch % Timestamp::k_ms_per_sec);
    if (sec != t_last_sec)
    {
        t_last_sec = sec;
        struct tm tm_time;
        ::gmtime_r(&sec, &tm_time);

        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17);
    }

    Fmt us(".%06d ", ms);
    assert(us.length() == 8);
    stream_ << T(t_time, 17) << T(us.data(), 8);
}

void Logger::default_output(const char *msg, int len)
{
    size_t n = fwrite(msg, 1, len, stdout);
}

void Logger::default_flush()
{
    fflush(stdout);
}

Logger::LogLevel &Logger::log_level_()
{
#ifdef RELEASE
    static LogLevel log_level = LogLevel::INFO;
#else
    static LogLevel log_level = LogLevel::DEBUG;
#endif
    return log_level;
}

Logger::OutputFunc &Logger::output_func_()
{
    static OutputFunc output_func = Logger::default_output;
    return output_func;
}

Logger::FlushFunc &Logger::flush_func_()
{
    static FlushFunc flush_func = Logger::default_flush;
    return flush_func;
}

void Logger::set_log_output(const Logger::OutputFunc &output_func, const Logger::FlushFunc &flush_func)
{
    output_func_() = output_func;
    flush_func_() = flush_func;
}

void Logger::set_log_output(Logger::OutputFunc &&output_func, Logger::FlushFunc &&flush_func)
{
    output_func_() = std::move(output_func);
    flush_func_() = std::move(flush_func);
}





