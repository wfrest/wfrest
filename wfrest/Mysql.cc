#include "workflow/WFTaskFactory.h"
#include "workflow/Workflow.h"
#include "wfrest/Mysql.h"
#include "wfrest/Logger.h"
#include "wfrest/MysqlUtil.h"

using namespace wfrest;
using namespace protocol;

// init static member var
std::atomic<int64_t> MySQL::id_{0};

void MySQL::query(const std::string &sql, const MySQLFunc &mysql_func)
{
    WFMySQLTask *task = conn_->create_query_task(sql,
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

    if(head_task_ == nullptr)
    {
        head_task_ = task;
        SeriesWork *series = Workflow::create_series_work(head_task_, nullptr);
    }
    else 
    {
        **head_task_ << task;
    }
}

void MySQL::start()
{
    series_of(head_task_)->start();
}

MySQL::MySQL(const std::string& url)
{
    conn_ = new WFMySQLConnection(id_++);
    conn_->init(url);
}

MySQL::~MySQL()
{
    conn_->deinit();
    delete conn_;
}

