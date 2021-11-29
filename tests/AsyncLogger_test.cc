#include "AsyncFileLogger.h"
#include "Logger.h"

using namespace wfrest;

int main()
{
    AsyncFileLogger logger;
    logger.start();

    Logger::set_output([&](const char *msg, int len)
                       {
                           logger.output(msg, len);
                       }
    );

    int i = 0;
    while (i < 1000000)
    {
        ++i;
        if (i % 100 == 0)
        {
            LOG_ERROR << "this is the " << i << "th log";
            continue;
        }
        LOG_INFO << "this is the " << i << "th log";
        ++i;
        LOG_DEBUG << "this is the " << i << "th log";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}