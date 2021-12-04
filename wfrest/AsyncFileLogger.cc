#include "wfrest/AsyncFileLogger.h"

using namespace wfrest;

// static member init
uint64_t AsyncFileLogger::LogFile::file_seq_ = 0;

AsyncFileLogger::AsyncFileLogger()
        : wait_group_(1),
          log_buf_(new Buffer),
          next_buf_(new Buffer),
          tmp_buf1_(new Buffer),
          tmp_buf2_(new Buffer),
          running_(false)
{
    log_buf_->bzero();
    next_buf_->bzero();
    tmp_buf1_->bzero();
    tmp_buf2_->bzero();
    bufs_.reserve(16);
    bufs_to_write_.reserve(16);
}

AsyncFileLogger::~AsyncFileLogger()
{
    if (running_)
    {
        stop();
    }
}

void AsyncFileLogger::write_log_to_file(const char *buf, int len)
{
    p_log_file_->write_log(buf, len);
    if (p_log_file_->length() > Logger::get_logger_settings()->roll_size)
    {
        p_log_file_.reset();
    }
}

void AsyncFileLogger::start()
{
    running_ = true;
    fprintf(stderr, "Async File Logger start...\n");
    thread_ = std::thread(std::bind(&AsyncFileLogger::thread_func, this));
    wait_group_.wait();
}

void AsyncFileLogger::thread_func()
{
    wait_group_.done();
    auto *log_settings = Logger::get_logger_settings();

    p_log_file_ = std::unique_ptr<LogFile>(
            new LogFile(log_settings->file_path,
                        log_settings->file_base_name,
                        log_settings->file_extension)
    );

    while (running_)
    {
        wait_for_buf();
        erase_extra_buf();
        write_bufs();
        put_back_tmp_buf();
        bufs_to_write_.clear();
        p_log_file_->flush();
    }
}

void AsyncFileLogger::output(const char *msg, int len)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_buf_->available() > len)
    {
        log_buf_->append(msg, len);
    } else
    {
        bufs_.emplace_back(std::move(log_buf_));
        if (next_buf_)
        {
            log_buf_ = std::move(next_buf_);
        } else
        {
            log_buf_.reset(new Buffer);
        }
        log_buf_->append(msg, len);
        cv_.notify_one();
    }
}

void AsyncFileLogger::wait_for_buf()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (bufs_.empty())
    {
        cv_.wait_for(lock, Logger::get_logger_settings()->flush_interval);
    }
    bufs_.emplace_back(std::move(log_buf_));
    log_buf_ = std::move(tmp_buf1_);   // put a tmp buffer to a new log_buf
    bufs_to_write_.swap(bufs_);
    if (!next_buf_)
    {
        next_buf_ = std::move(tmp_buf2_);
    }
}

void AsyncFileLogger::erase_extra_buf()
{
    // The production speed exceeds the consumption speed,
    // which will cause the accumulation of data in the memory
    // If the messages pile up, delete the extra data
    if (bufs_to_write_.size() > 25)
    {
        char buf[256];
        snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
                 Timestamp::now().to_format_str().c_str(),
                 bufs_to_write_.size() - 2);   // reserve 2 buffers
        fputs(buf, stderr);
        write_log_to_file(buf, strlen(buf));
        bufs_to_write_.erase(bufs_to_write_.begin() + 2, bufs_to_write_.end());
    }
}

void AsyncFileLogger::write_bufs()
{
    for (const auto &buf: bufs_to_write_)
    {
        write_log_to_file(buf->data(), buf->length());
    }
    if (bufs_to_write_.size() > 2)
    {
        bufs_to_write_.resize(2);
    }
}

void AsyncFileLogger::put_back_tmp_buf()
{
    if (!tmp_buf1_)
    {
        tmp_buf1_ = std::move(bufs_to_write_.back());
        bufs_to_write_.pop_back();
        tmp_buf1_->reset();
    }
    if (!tmp_buf2_)
    {
        tmp_buf2_ = std::move(bufs_to_write_.back());
        bufs_to_write_.pop_back();
        tmp_buf2_->reset();
    }
}

void AsyncFileLogger::stop()
{
    running_ = false;
    cv_.notify_one();
    thread_.join();
}

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
        snprintf(seq, sizeof seq, "_%06llu",
                 static_cast<long long unsigned int>(file_seq_ % 1000000));
        ++file_seq_;
        std::string new_name = file_path_ + file_base_name_ + "_" +
                               create_time_.to_format_str("%Y-%m-%d_%X") +
                               std::string(seq) + file_extension_;
                               
        rename(file_full_name_.c_str(), new_name.c_str());
    }
}

uint64_t AsyncFileLogger::LogFile::length()
{
    if (fp_)
        return ftell(fp_);
    return 0;
}

void AsyncFileLogger::LogFile::flush()
{
    if (fp_)
    {
        fflush(fp_);
    }
}

void AsyncFileLogger::LogFile::write_log(const char *buf, int len)
{
    if (fp_)
    {
        // https://stackoverflow.com/questions/10564562/fwrite-effect-of-size-and-count-on-performance
        // fprintf(stderr, "write %d bytes to file\n", len);
        fwrite(buf, 1, len, fp_);
    }
}
