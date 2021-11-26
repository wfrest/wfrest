#ifndef WFREST_ASYNCFILELOGGER_H_
#define WFREST_ASYNCFILELOGGER_H_

#include "workflow/WFFacilities.h"

#include <string>
#include <condition_variable>
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <chrono>

#include "Logger.h"

namespace wfrest
{
class AsyncFileLogger
{
public:
    AsyncFileLogger();

    ~AsyncFileLogger();

    void set_file_name(const std::string &base_name,
                       const std::string &extension = ".log",
                       const std::string &path = "./");

    void start();

    void stop();

    void output(const char *msg, int len);

private:
    using Buffer = detail::FixedBuffer<detail::k_large_buf>;
    using BufferVector = std::vector<std::unique_ptr<Buffer> >;
    using BufferPtr = BufferVector::value_type;

    class LogFile
    {
    public:
        LogFile(const std::string &file_path,
                const std::string &file_base_name,
                const std::string &file_extension);

        ~LogFile();

        void write_log(const char *buf, int len);

        uint64_t length();

        void flush();

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
    void write_log_to_file(const char *buf, int len);

    void thread_func();

    void wait_for_buffer();

    void erase_extra_buffer();

    void bufs_write();

    void put_back_tmp_buf();

private:
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    WFFacilities::WaitGroup wait_group_;
    // front end
    BufferPtr log_buf_;
    BufferPtr next_buf_;
    BufferVector bufs_;
    // back end
    BufferPtr tmp_buf1_;
    BufferPtr tmp_buf2_;
    BufferVector bufs_to_write_;

    std::atomic<bool> running_;
    std::unique_ptr<LogFile> p_log_file_;
    std::string file_path_ = "./";
    std::string file_base_name_ = "wfrest";
    std::string file_extension_ = ".log";
    uint64_t roll_size_ = 20 * 1024 * 1024;
    static const std::chrono::seconds k_flush_interval;
};

} // namespace wfrest

#endif // WFREST_ASYNCFILELOGGER_H_