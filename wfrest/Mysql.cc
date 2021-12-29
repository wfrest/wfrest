#include "workflow/WFTaskFactory.h"

#include "wfrest/Mysql.h"
#include "wfrest/BlockSeries.h"

using namespace wfrest;

void Mysql::execute(const std::string &sql, mysql_callback_t callback)
{
    WFMySQLTask *task = WFTaskFactory::create_mysql_task(url_, 0, callback);
    task->get_req()->set_query(sql);
    block_series_->push_back(task);
}

Mysql::Mysql(const std::string& url)
    : url_(url)
{
    WFEmptyTask *empty_task = WFTaskFactory::create_empty_task();
    block_series_ = new BlockSeries(empty_task, std::to_string(id_++));
    block_series_->start();
}

Mysql::~Mysql()
{
    delete block_series_;
}



