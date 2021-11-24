#ifndef _ASYNCFILELOGGER_H_
#define _ASYNCFILELOGGER_H_

#include <string>
#include <condition_variable>
#include <vector>
#include <queue>
#include <memory>
#include <thread>

#include "Logger.h"

namespace wfrest
{
class AsyncFileLogger
{
public:
    AsyncFileLogger();

    ~AsyncFileLogger();

    void set_file_name();
private:
    using Buffer = detail::FixedBuffer<detail::k_large_buf>;
    using BufferQueue = std::queue<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferQueue::value_type;

    class LogFile
    {
    public:
        LogFile(const std::string &file_path,
                const std::string &file_base_name,
                const std::string &file_extension);

        ~LogFile();

        void write_log(const BufferPtr buf);
        uint64_t length();
    private:
        FILE *fp_ = nullptr;
        Timestamp create_time_;
        std::string file_full_name_;
        std::string file_path_;
        std::string file_base_name_;
        std::string file_extension_;
        static uint64_t file_seq_;
    };

private:
    void write_log_to_file(BufferPtr buf);

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    BufferPtr log_buf_;
    BufferPtr next_buf_;
    BufferQueue bufs_;
    std::unique_ptr<std::thread> p_thread_;
    bool stop_flag = false;
    std::unique_ptr<LogFile> p_log_file_;
    std::string file_path_ = "./";
    std::string file_base_name_ = "wfrest";
    std::string file_extension_ = ".log";
    uint64_t size_limit_ = 20 * 1024 * 1024;
};

} // namespace wfrest



#endif // _ASYNCFILELOGGER_H_