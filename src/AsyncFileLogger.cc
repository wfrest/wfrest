#include "AsyncFileLogger.h"

using namespace wfrest;

AsyncFileLogger::AsyncFileLogger()
{
    log_buf_->bzero();
    next_buf_->bzero();
}

AsyncFileLogger::~AsyncFileLogger()
{
    stop_flag = true;
    if (p_thread_)
    {
        cv_.notify_all();
        p_thread_->join();
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (log_buf_->length() > 0)
        {
            bufs_.emplace(std::move(log_buf_));
        }
        while (!bufs_.empty())
        {
            BufferPtr tmp_buf = std::move(bufs_.front());
            bufs_.pop();
            write_log_to_file(std::move(tmp_buf));
        }
    }
}

void AsyncFileLogger::write_log_to_file(BufferPtr buf)
{
    if (!p_log_file_)
    {
        p_log_file_ = std::unique_ptr<LogFile>(
                new LogFile(file_path_, file_base_name_, file_extension_)
        );
    }
    p_log_file_->write_log(std::move(buf));
    if (p_log_file_->length() > size_limit_)
    {
        p_log_file_.reset();
    }
}

uint64_t AsyncFileLogger::LogFile::file_seq_ = 0;

AsyncFileLogger::LogFile::LogFile(const std::string &file_path,
                                  const std::string &file_base_name,
                                  const std::string &file_extension)
        : create_time_(Timestamp::now()),
          file_path_(file_path),
          file_base_name_(file_base_name),
          file_extension_(file_extension)
{
    file_full_name_ = file_path + file_base_name + file_extension;
    fp_ = fopen(file_full_name_.c_str(), "a");
    if (fp_ == nullptr)
    {
        fprintf(stderr, "%s\n", strerror(errno));
    }
}

AsyncFileLogger::LogFile::~LogFile()
{
    if (fp_)
    {
        fclose(fp_);
        char seq[12];
        snprintf(seq, sizeof(seq), ".%06llu",
                 static_cast<long long unsigned int>(file_seq_ % 1000000));
        ++file_seq_;
        std::string new_name = file_path_ + file_base_name_ + "." +
                               create_time_.to_format_str() + std::string(seq) + file_extension_;
        rename(file_full_name_.c_str(), new_name.c_str());
    }
}

void AsyncFileLogger::LogFile::write_log(const AsyncFileLogger::BufferPtr buf)
{
    if (fp_)
    {
        // fprintf(stderr, "write %d bytes to file\n", buf->length());
        fwrite(buf->data(), 1, buf->length(), fp_);
    }
}

uint64_t AsyncFileLogger::LogFile::length()
{
    if (fp_)
        return ftell(fp_);
    return 0;
}
