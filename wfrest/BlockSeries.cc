#include "wfrest/BlockSeries.h"

using namespace wfrest;

BlockSeries::BlockSeries(SubTask *first, const std::string &name) 
    : counter_name_(name)
{
    series_ = Workflow::create_series_work(first, nullptr);
    WFCounterTask *counter = WFTaskFactory::create_counter_task(counter_name_, 1, nullptr);
    series_->push_back(counter);
}

BlockSeries::~BlockSeries()
{
    WFTaskFactory::count_by_name(counter_name_);
}

void BlockSeries::start()
{
    series_->start();
}

void BlockSeries::push_back(SubTask *task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        WFCounterTask *counter = WFTaskFactory::create_counter_task(counter_name_, 1, nullptr);
        series_->push_back(task);
        series_->push_back(counter);
    }
    WFTaskFactory::count_by_name(counter_name_);
}