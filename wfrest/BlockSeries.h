#ifndef WFREST_BLOCKSERIES_H_
#define WFREST_BLOCKSERIES_H_

#include <string>
#include <mutex>
#include "workflow/Workflow.h"
#include "workflow/WFTaskFactory.h"

namespace wfrest
{

class BlockSeries
{
public:
    BlockSeries(SubTask *first, const std::string &name);

    ~BlockSeries();

    void start();

    void push_back(SubTask *task);

private:
    SeriesWork *series_;
    std::string counter_name_;
    std::mutex mutex_;
};

} // namespace wfrest



#endif // WFREST_BLOCKSERIES_H_