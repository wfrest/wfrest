//
// Created by Chanchan on 11/18/21.
//

#include "Logger.h"

namespace wfrest
{
// default write to stdout
// Can set to file
void defaultOutput(const char *msg, int len)
{
    size_t n = fwrite(msg, 1, len, stdout);
}

void defaultFlush()
{
    fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

}  // namespace wfrest

using namespace wfrest;


void Logger::setOutput(Logger::OutputFunc out)
{
    g_output = out;
}

void Logger::setFlush(Logger::FlushFunc flush)
{
    g_flush = flush;
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
    impl_.stream_ << func << ' ';
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
    g_output(buf.data(), buf.length());
    if (impl_.level_ == FATAL)
    {
        g_flush();
        abort();
    }
}







