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
class Mysql : public Noncopyable
{
public:
    void execute(const std::string &sql, mysql_callback_t callback);

    // void reset(const std::string &url);

public:
    explicit Mysql(const std::string &url);

    ~Mysql();

private:
    std::string url_;
    BlockSeries *block_series_;
    std::atomic<int64_t> id_;
};


} // namespace wfrest






#endif // WFREST_MYSQL_H_