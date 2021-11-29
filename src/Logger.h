// Modified from muduo
#ifndef WFREST_LOGGER_H_
#define WFREST_LOGGER_H_

#include <functional>

#include "LogStream.h"
#include "Timestamp.h"

namespace wfrest
{

enum class LogLevel
{
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

struct LoggerSettings
{
    LogLevel level;
    bool log_in_file;
    const char *file_path;
    const char *file_base_name;
    const char *file_extension;
    uint64_t roll_size;
    std::chrono::seconds flush_interval;
};

static constexpr struct LoggerSettings LOGGER_SETTINGS_DEFAULT =
{
    .level = LogLevel::INFO,
    .log_in_file = false,
    .file_path = "./",
    .file_base_name = "wfrest",
    .file_extension = ".log",
    .roll_size = 20 * 1024 * 1024,
    .flush_interval = std::chrono::seconds(3),
};

extern void WFREST_init_logger(struct LoggerSettings *settings);

class Logger
{
public:
    // Two std::funcion set output location
    using OutputFunc = std::function<void(const char *msg, int len)>;
    using FlushFunc = std::function<void()>;

    static void set_output(const OutputFunc &output_func)
    { output_func_() = output_func; }

    static void set_output(OutputFunc &&output_func)
    { output_func_() = std::move(output_func); }

    static void set_output(const OutputFunc &output_func, const FlushFunc &flush_func);

    static void set_output(OutputFunc &&output_func, FlushFunc &&flush_func);

    // compile time calculation of basename of source file
    class SourceFile
    {
    public:
        template<int N>
        SourceFile(const char (&arr)[N]);

        explicit SourceFile(const char *filename);

    public:
        const char *data_;
        int size_;
    };

public:
    Logger(SourceFile file, int line);

    Logger(SourceFile file, int line, LogLevel level);

    Logger(SourceFile file, int line, LogLevel level, const char *func);

    Logger(SourceFile file, int line, bool toAbort);

    ~Logger();

    LogStream &stream()
    { return impl_.stream_; }

    static LogLevel log_level();

protected:
    // default write to stdout
    // Can set to file
    static void default_output(const char *msg, int len);

    static void file_output(const char *msg, int len);

    static void default_flush();

    static OutputFunc &output_func_();

    static FlushFunc &flush_func_();

private:
    class Impl
    {
    public:
        Impl(LogLevel level, int old_errno, const SourceFile &file, int line);

        void formatTime();

    public:
        Timestamp time_;
        LogStream stream_;
        LogLevel level_;
        int line_;
        SourceFile basename_;
    } impl_;
};

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

#define LOG_TRACE if (wfrest::Logger::log_level() <= wfrest::LogLevel::TRACE) \
  wfrest::Logger(__FILE__, __LINE__, wfrest::LogLevel::TRACE, __func__).stream()
#define LOG_DEBUG if (wfrest::Logger::log_level() <= wfrest::LogLevel::DEBUG) \
  wfrest::Logger(__FILE__, __LINE__, wfrest::LogLevel::DEBUG, __func__).stream()
#define LOG_INFO if (wfrest::Logger::log_level() <= wfrest::LogLevel::INFO) \
  wfrest::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN wfrest::Logger(__FILE__, __LINE__, wfrest::LogLevel::WARN).stream()
#define LOG_ERROR wfrest::Logger(__FILE__, __LINE__, wfrest::LogLevel::ERROR).stream()
#define LOG_FATAL wfrest::Logger(__FILE__, __LINE__, wfrest::LogLevel::FATAL).stream()
#define LOG_SYSERR wfrest::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL wfrest::Logger(__FILE__, __LINE__, true).stream()

// note: LOG_TRACE << "the server has read the client message";
// equals wfrest::Logger(__FILE__,__LINE__,wfrest::Logger::TRACE,__func__).stream()<<"the server has read the client message";

#define LOGGER(settings) WFREST_init_logger(settings)

}  // namespace wfrest



#endif // WFREST_LOGGER_H_
