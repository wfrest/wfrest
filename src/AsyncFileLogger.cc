#include "AsyncFileLogger.h"

using namespace wfrest;

// static member init
uint64_t AsyncFileLogger::LogFile::file_seq_ = 0;
const std::chrono::seconds AsyncFileLogger::k_flush_interval = std::chrono::seconds(1);

AsyncFileLogger::AsyncFileLogger()
        : wait_group_(1),
          log_buf_(new Buffer),
          next_buf_(new Buffer),
          bufs_(16),
          tmp_buf1_(new Buffer),
          tmp_buf2_(new Buffer),
          bufs_to_write_(16)
{
    log_buf_->bzero();
    next_buf_->bzero();
    tmp_buf1_->bzero();
    tmp_buf2_->bzero();
}

AsyncFileLogger::~AsyncFileLogger()
{
    if (running_)
    {
        stop();
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
    if (p_log_file_->length() > roll_size_)
    {
        p_log_file_.reset();
    }
}

void AsyncFileLogger::set_file_name(const std::string &base_name,
                                    const std::string &extension,
                                    const std::string &path)
{
    file_base_name_ = base_name;
    extension[0] == '.' ? file_extension_ = extension : file_extension_ = "." + extension;
    file_path_ = path;
    if (file_path_.length() == 0)
    {
        file_path_ = "./";
    }
    if (file_path_[file_path_.length() - 1] != '/')
    {
        file_path_ = file_path_ + "/";
    }
}

void AsyncFileLogger::start()
{
    running_ = true;
    thread_ = std::thread(std::bind(&AsyncFileLogger::thread_func, this));
    wait_group_.wait();
}

void AsyncFileLogger::thread_func()
{
    wait_group_.done();
    while (running_)
    {
        wait_for_buffer();
        erase_extra_buffer();
        bufs_write();
        put_back_tmp_buf();
        bufs_to_write_.clear();
        if (p_log_file_)
        {
            p_log_file_->flush();
        }
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

void AsyncFileLogger::wait_for_buffer()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (bufs_.empty())
    {
        cv_.wait_for(lock, k_flush_interval);
    }
    bufs_.emplace_back(std::move(log_buf_));
    log_buf_ = std::move(tmp_buf1_);   // put a tmp buffer to a new log_buf
    bufs_to_write_.swap(bufs_);
    if (!next_buf_)
    {
        next_buf_ = std::move(tmp_buf2_);
    }
}

void AsyncFileLogger::erase_extra_buffer()
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
        BufferPtr tmp_buf;
        tmp_buf->append(buf, strlen(buf));
        write_log_to_file(std::move(tmp_buf));
        bufs_to_write_.erase(bufs_to_write_.begin() + 2, bufs_to_write_.end());
    }
}

void AsyncFileLogger::bufs_write()
{
    for (auto &buf: bufs_to_write_)
    {
        write_log_to_file(std::move(buf));
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

void AsyncFileLogger::LogFile::flush()
{
    if (fp_)
    {
        fflush(fp_);
    }
}
