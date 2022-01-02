#include "workflow/WFTaskFactory.h"
#include "wfrest/Mysql.h"
#include "wfrest/BlockSeries.h"
#include "wfrest/Logger.h"

using namespace wfrest;
using namespace protocol;

void MySQL::execute(const std::string &sql, const MySQLFunc &mysql_func)
{
    WFMySQLTask *task = WFTaskFactory::create_mysql_task(url_, 
                                                        0, 
                                                        [mysql_func](WFMySQLTask *task)
    {
        Status status;
        if (task->get_state() != WFT_STATE_SUCCESS)
        {
            LOG_ERROR << "error msg:" 
                    << WFGlobal::get_error_string(task->get_state(),
                                                task->get_error());
            return;
        } else
        {
            status.state = task->get_state();
            status.error = task->get_error();
        }
        MySQLResponse *resp = task->get_resp();
        MySQLResultCursor cursor(resp);
        if(mysql_func)
            mysql_func(cursor, status);
    });

    task->get_req()->set_query(sql);
    block_series_->push_back(task);
}


MySQL::MySQL(const std::string& url)
    : url_(url)
{
    WFEmptyTask *empty_task = WFTaskFactory::create_empty_task();
    block_series_ = new BlockSeries(empty_task, std::to_string(id_++));
    block_series_->start();
}

MySQL::~MySQL()
{
    delete block_series_;
}



