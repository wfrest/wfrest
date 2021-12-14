// Modified from muduo
#ifndef WFREST_LOGGER_H_
#define WFREST_LOGGER_H_

#include <functional>

#include "wfrest/LogStream.h"
#include "wfrest/Timestamp.h"

namespace wfrest
{

class AsyncFileLogger;

enum class LogLevel
{
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class LoggerSetting
{
public:
    LoggerSetting &set_level(LogLevel level)
    {
        level_ = level;
        return *this;
    }

    LoggerSetting &set_log_in_console(bool log_in_console)
    {
        log_in_console_ = log_in_console;
        return *this;
    }

    LoggerSetting &set_log_in_file(bool log_in_file)
    {
        log_in_file_ = log_in_file;
        return *this;
    }

    LoggerSetting &set_file_path(const std::string &file_path)
    {
        file_path_ = file_path;
        return *this;
    }

    LoggerSetting &set_file_path(std::string &&file_path)
    {
        file_path_ = std::move(file_path);
        return *this;
    }

    LoggerSetting &set_file_base_name(const std::string &file_base_name)
    {
        file_base_name_ = file_base_name;
        return *this;
    }

    LoggerSetting &set_file_base_name(std::string &&file_base_name)
    {
        file_base_name_ = std::move(file_base_name);
        return *this;
    }

    LoggerSetting &set_file_extension(const std::string &file_extension)
    {
        file_extension_ = file_extension;
        return *this;
    }

    LoggerSetting &set_file_extension(std::string &&file_extension)
    {
        file_extension_ = std::move(file_extension);
        return *this;
    }

    LoggerSetting &set_roll_size(uint64_t roll_size)
    {
        roll_size_ = roll_size;
        return *this;
    }

    LoggerSetting &set_flush_interval(std::chrono::seconds flush_interval)
    {
        flush_interval_ = flush_interval;
        return *this;
    }

public: 
    LogLevel level() const 
    { return level_; }

    bool is_log_in_console() const
    { return log_in_console_; }

    bool is_log_in_file() const
    { return log_in_file_; }

    const std::string &file_path() const
    { return file_path_; }

    const std::string &file_base_name() const
    { return file_base_name_; }

    const std::string &file_extension() const
    { return file_extension_; }

    uint64_t roll_size() const 
    { return roll_size_; }

    std::chrono::seconds flush_interval() const 
    { return flush_interval_; }

private:
    LogLevel level_ = LogLevel::INFO;
    bool log_in_console_ = true;
    bool log_in_file_ = false;
    std::string file_path_ = "./";
    std::string file_base_name_ = "wfrest";
    std::string file_extension_ = ".log";
    uint64_t roll_size_ = 20 * 1024 * 1024;
    std::chrono::seconds flush_interval_ = std::chrono::seconds(3);
};

void logger_init(LoggerSetting &&setting);

class Logger : public Noncopyable
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

    static LogLevel log_level();

    static const LoggerSetting &get_setting() 
    { return setting_; }

    static void set_setting(const LoggerSetting &setting)
    { setting_ = setting; }

    static void set_setting(LoggerSetting &&setting)
    { setting_ = std::move(setting); }

    static AsyncFileLogger *get_async_file_logger();

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

protected:
    // default write to stderr
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

    static LoggerSetting setting_;
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

#define LOGGER(setting) logger_init(std::move(setting))

}  // namespace wfrest



#endif // WFREST_LOGGER_H_
