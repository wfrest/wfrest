#ifndef WFREST_MYSQL_H_
#define WFREST_MYSQL_H_

#include "workflow/WFTaskFactory.h"
#include "workflow/MySQLResult.h"
#include <atomic>
#include <string>
#include "wfrest/Noncopyable.h"

namespace wfrest
{

class BlockSeries;

class MySQL : public Noncopyable
{
public:
    struct Status
    {
        int state;
        int error;
    };
    using MySQLFunc = std::function<void(protocol::MySQLResultCursor &cursor, 
                                        const Status &status)>;

    void execute(const std::string &sql, const MySQLFunc &mysql_func);

    // void reset(const std::string &url);

public:
    explicit MySQL(const std::string &url);

    ~MySQL();

private:
    std::string url_;
    BlockSeries *block_series_;
    std::atomic<int64_t> id_;
};

} // namespace wfrest

#endif // WFREST_MYSQL_H_