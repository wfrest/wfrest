//
// Created by Chanchan on 11/18/21.
//

#include "Logger.h"

namespace wfrest
{
    // default write to stdout
    // Can set to file
    void defaultOutput(const char* msg, int len)
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
