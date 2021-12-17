#include "workflow/WFTaskFactory.h"

namespace wfrest
{

namespace detail
{

// In order to reduce the use of generic programming, add some redundant code
template<typename Tuple>
WFGoTask *aop_process(const Handler &handler,
                      const HttpReq *req,
                      HttpResp *resp,
                      Tuple &tp)
{
    bool ret = aop_before(req, resp, tp);
    if (!ret)
    {
        LOG_DEBUG << "before wrong";
        return nullptr;
    }

    handler(req, resp);

    ret = aop_after(req, resp, tp);
    if (!ret)
    {
        LOG_DEBUG << "after wrong";
    }
    return nullptr;
}

template<typename Tuple>
WFGoTask *aop_process(const SeriesHandler &handler,
                      const HttpReq *req,
                      HttpResp *resp,
                      SeriesWork *series,
                      Tuple &tp)
{
    bool ret = aop_before(req, resp, tp);
    if (!ret)
    {
        LOG_DEBUG << "before wrong";
        return nullptr;
    }


    handler(req, resp, series);

    ret = aop_after(req, resp, tp);
    if (!ret)
    {
        LOG_DEBUG << "after wrong";
    }
    return nullptr;
}

template<typename Tuple>
WFGoTask *aop_compute_process(const Handler &handler,
                              int compute_queue_id,
                              const HttpReq *req,
                              HttpResp *resp,
                              Tuple *ptp)
{
    bool ret = aop_before(req, resp, *ptp);
    if (!ret)
    {
        LOG_DEBUG << "before wrong";
        return nullptr;
    }

    WFGoTask *go_task = WFTaskFactory::create_go_task(
            "wfrest" + std::to_string(compute_queue_id),
            handler,
            req,
            resp);

    go_task->set_callback([req, resp, ptp](WFGoTask *)
                          {
                              bool ret = aop_after(req, resp, *ptp);
                              if (!ret)
                              {
                                  LOG_DEBUG << "after wrong";
                              }
                              delete ptp;
                          });
    return go_task;
}


template<typename Tuple>
WFGoTask *aop_compute_process(const SeriesHandler &handler,
                              int compute_queue_id,
                              const HttpReq *req,
                              HttpResp *resp,
                              SeriesWork *series,
                              Tuple *ptp)
{
    bool ret = aop_before(req, resp, *ptp);
    if (!ret)
    {
        LOG_DEBUG << "before wrong";
        return nullptr;
    }

    WFGoTask *go_task = WFTaskFactory::create_go_task(
            "wfrest" + std::to_string(compute_queue_id),
            handler,
            req,
            resp,
            series);

    go_task->set_callback([req, resp, ptp](WFGoTask *)
                          {
                              bool ret = aop_after(req, resp, *ptp);
                              if (!ret)
                              {
                                  LOG_DEBUG << "after wrong";
                              }
                              delete ptp;
                          });
    return go_task;
}


}  // namespace detail

template<typename... AP>
void BluePrint::GET(const char *route, const Handler &handler, const AP &... ap)
{
    WrapHandler wrap_handler =
            [handler, ap...](const HttpReq *req,
                             HttpResp *resp,
                             SeriesWork *) -> WFGoTask *
            {
                std::tuple<AP...> tp(std::move(ap)...);
                WFGoTask *go_task = detail::aop_process(handler,
                                                        req,
                                                        resp,
                                                        tp);
                return go_task;
            };

    router_.handle(route, -1, wrap_handler, Verb::GET);
}

template<typename... AP>
void BluePrint::GET(const char *route, int compute_queue_id,
                    const Handler &handler, const AP &... ap)
{
    WrapHandler wrap_handler =
            [handler, compute_queue_id, ap...](HttpReq *req,
                                               HttpResp *resp,
                                               SeriesWork *) -> WFGoTask *
            {
                auto *ptp = new std::tuple<AP...>(std::move(ap)...);
                WFGoTask *go_task = detail::aop_compute_process(handler,
                                                                compute_queue_id,
                                                                req,
                                                                resp,
                                                                ptp);
                return go_task;
            };

    router_.handle(route, compute_queue_id, wrap_handler, Verb::GET);
}

template<typename... AP>
void BluePrint::POST(const char *route, const Handler &handler, const AP &... ap)
{
    WrapHandler wrap_handler =
            [handler, ap...](const HttpReq *req,
                             HttpResp *resp,
                             SeriesWork *) -> WFGoTask *
            {
                std::tuple<AP...> tp(std::move(ap)...);
                WFGoTask *go_task = detail::aop_process(handler,
                                                        req,
                                                        resp,
                                                        tp);
                return go_task;
            };

    router_.handle(route, -1, wrap_handler, Verb::POST);
}

template<typename... AP>
void BluePrint::POST(const char *route, int compute_queue_id,
                     const Handler &handler, const AP &... ap)
{
    WrapHandler wrap_handler =
            [handler, compute_queue_id, ap...](HttpReq *req,
                                               HttpResp *resp,
                                               SeriesWork *) -> WFGoTask *
            {
                auto *ptp = new std::tuple<AP...>(std::move(ap)...);
                WFGoTask *go_task = detail::aop_compute_process(handler,
                                                                compute_queue_id,
                                                                req,
                                                                resp,
                                                                ptp);
                return go_task;
            };

    router_.handle(route, compute_queue_id, wrap_handler, Verb::POST);
}

template<typename... AP>
void BluePrint::GET(const char *route, const SeriesHandler &handler, const AP &... ap)
{
    WrapHandler wrap_handler =
            [handler, ap...](const HttpReq *req,
                             HttpResp *resp,
                             SeriesWork *series) -> WFGoTask *
            {
                std::tuple<AP...> tp(std::move(ap)...);
                WFGoTask *go_task = detail::aop_process(handler,
                                                        req,
                                                        resp,
                                                        series,
                                                        tp);
                return go_task;
            };

    router_.handle(route, -1, wrap_handler, Verb::GET);
}

template<typename... AP>
void BluePrint::GET(const char *route, int compute_queue_id,
                    const SeriesHandler &handler, const AP &... ap)
{
    WrapHandler wrap_handler =
            [handler, compute_queue_id, ap...](HttpReq *req,
                                               HttpResp *resp,
                                               SeriesWork *series) -> WFGoTask *
            {
                auto *ptp = new std::tuple<AP...>(std::move(ap)...);
                WFGoTask *go_task = detail::aop_compute_process(handler,
                                                                compute_queue_id,
                                                                req,
                                                                resp,
                                                                series,
                                                                ptp);
                return go_task;
            };

    router_.handle(route, compute_queue_id, wrap_handler, Verb::GET);
}

template<typename... AP>
void BluePrint::POST(const char *route, const SeriesHandler &handler, const AP &... ap)
{
    WrapHandler wrap_handler =
            [handler, ap...](const HttpReq *req,
                             HttpResp *resp,
                             SeriesWork *series) -> WFGoTask *
            {
                std::tuple<AP...> tp(std::move(ap)...);
                WFGoTask *go_task = detail::aop_process(handler,
                                                        req,
                                                        resp,
                                                        series,
                                                        tp);
                return go_task;
            };

    router_.handle(route, -1, wrap_handler, Verb::POST);
}

template<typename... AP>
void BluePrint::POST(const char *route, int compute_queue_id,
                     const SeriesHandler &handler, const AP &... ap)
{
    WrapHandler wrap_handler =
            [handler, compute_queue_id, ap...](HttpReq *req,
                                               HttpResp *resp,
                                               SeriesWork *series) -> WFGoTask *
            {
                auto *ptp = new std::tuple<AP...>(std::move(ap)...);
                WFGoTask *go_task = detail::aop_compute_process(handler,
                                                                compute_queue_id,
                                                                req,
                                                                resp,
                                                                series,
                                                                ptp);
                return go_task;
            };

    router_.handle(route, compute_queue_id, wrap_handler, Verb::POST);
}

} // namespace wfrest





