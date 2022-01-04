#ifndef WFREST_MYSQL_H_
#define WFREST_MYSQL_H_

#include "workflow/WFTaskFactory.h"
#include "workflow/MySQLResult.h"
#include "workflow/WFMySQLConnection.h"
#include <atomic>
#include <string>
#include "wfrest/Noncopyable.h"

namespace wfrest
{

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

    void query(const std::string &sql, const MySQLFunc &mysql_func);

    WFMySQLTask *front() { return head_task_; }

    void start();

public:
    explicit MySQL(const std::string &url);

    ~MySQL();

private: 
    WFMySQLTask *head_task_ = nullptr; 
    WFMySQLConnection *conn_;
    static std::atomic<int64_t> id_;
};

inline SeriesWork& operator << (SeriesWork& series, MySQL &mysql)
{
    SubTask *first_task = mysql.front();
    if(!first_task) return series;  
    SeriesWork *mysql_series = series_of(first_task);

    // because first task didn't count in series queue
    series.push_back(first_task);

    SubTask *task;
    while(task = mysql_series->pop())
    {
        series.push_back(task);
    }
	return series;
}

inline SeriesWork& operator << (SeriesWork& series, MySQL *mysql)
{
    SubTask *first_task = mysql->front();
    if(!first_task) return series;  
    SeriesWork *mysql_series = series_of(first_task);

    // because first task didn't count in series queue
    series.push_back(first_task);
    
    SubTask *task;
    while(task = mysql_series->pop())
    {
        fprintf(stderr, "1\n");
        series.push_back(task);
    }
	return series;
}

} // namespace wfrest

#endif // WFREST_MYSQL_H_