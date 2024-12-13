#include "BluePrint.h"
#include "HttpMsg.h"

using namespace wfrest;

void BluePrint::ROUTE(const std::string &route, const Handler &handler, Verb verb)
{
    WrapHandler wrap_handler =
            [handler](const HttpReq *req,
                            HttpResp *resp,
                            SeriesWork *) -> WFGoTask *
            {
                GlobalAspect *global_aspect = GlobalAspect::get_instance();
                for(auto asp : global_aspect->aspect_list)
                {
                    auto ret = asp->before(req, resp);
                    if(!ret) return nullptr;
                }

                handler(req, resp);

                if(!global_aspect->aspect_list.empty())
                {
                    HttpServerTask *server_task = task_of(resp);
                    server_task->add_callback([req, resp, global_aspect](HttpTask *)
                    {
                        for(auto it = global_aspect->aspect_list.rbegin(); it != global_aspect->aspect_list.rend(); ++it)
                        {
                            (*it)->after(req, resp);
                        }
                    });
                }
                return nullptr;
            };

    router_.handle(route, -1, wrap_handler, verb);
}

void BluePrint::ROUTE(const std::string &route, int compute_queue_id, const Handler &handler, Verb verb)
{
    WrapHandler wrap_handler =
            [handler, compute_queue_id](HttpReq *req,
                                        HttpResp *resp,
                                        SeriesWork *) -> WFGoTask *
            {
                GlobalAspect *global_aspect = GlobalAspect::get_instance();
                for(auto asp : global_aspect->aspect_list)
                {
                    auto ret = asp->before(req, resp);
                    if(!ret) return nullptr;
                }
                WFGoTask *go_task = WFTaskFactory::create_go_task(
                        "wfrest" + std::to_string(compute_queue_id),
                        handler,
                        req,
                        resp);
                if(!global_aspect->aspect_list.empty())
                {
                    HttpServerTask *server_task = task_of(resp);
                    server_task->add_callback([req, resp, global_aspect](HttpTask *)
                    {
                        for(auto it = global_aspect->aspect_list.rbegin(); it != global_aspect->aspect_list.rend(); ++it)
                        {
                            (*it)->after(req, resp);
                        }
                    });
                }
                return go_task;
            };

    router_.handle(route, compute_queue_id, wrap_handler, verb);
}

void BluePrint::ROUTE(const std::string &route, const Handler &handler, const std::vector<std::string> &methods)
{
    for(const auto &method : methods)
    {
        this->ROUTE(route, handler, str_to_verb(method));
    }
}

void BluePrint::ROUTE(const std::string &route, int compute_queue_id, const Handler &handler, const std::vector<std::string> &methods)
{
    for(const auto &method : methods)
    {
        this->ROUTE(route, compute_queue_id, handler, str_to_verb(method));
    }
}

void BluePrint::GET(const std::string &route, const Handler &handler)
{
    this->ROUTE(route, handler, Verb::GET);
}

void BluePrint::GET(const std::string &route, int compute_queue_id, const Handler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::GET);
}

void BluePrint::POST(const std::string &route, const Handler &handler)
{
    this->ROUTE(route, handler, Verb::POST);
}

void BluePrint::POST(const std::string &route, int compute_queue_id, const Handler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::POST);
}

void BluePrint::DELETE(const std::string &route, const Handler &handler)
{
    this->ROUTE(route, handler, Verb::DELETE);
}

void BluePrint::DELETE(const std::string &route, int compute_queue_id, const Handler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::DELETE);
}

void BluePrint::PATCH(const std::string &route, const Handler &handler)
{
    this->ROUTE(route, handler, Verb::PATCH);
}

void BluePrint::PATCH(const std::string &route, int compute_queue_id, const Handler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::PATCH);
}

void BluePrint::PUT(const std::string &route, const Handler &handler)
{
    this->ROUTE(route, handler, Verb::PUT);
}

void BluePrint::PUT(const std::string &route, int compute_queue_id, const Handler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::PUT);
}

void BluePrint::HEAD(const std::string &route, const Handler &handler)
{
    this->ROUTE(route, handler, Verb::HEAD);
}

void BluePrint::HEAD(const std::string &route, int compute_queue_id, const Handler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::HEAD);
}

void BluePrint::ROUTE(const std::string &route, const SeriesHandler &handler, Verb verb)
{
    WrapHandler wrap_handler =
            [handler](const HttpReq *req,
                      HttpResp *resp,
                      SeriesWork *series) -> WFGoTask *
            {
                GlobalAspect *global_aspect = GlobalAspect::get_instance();
                for(auto asp : global_aspect->aspect_list)
                {
                    auto ret = asp->before(req, resp);
                    if(!ret) return nullptr;
                }
                handler(req, resp, series);
                if(!global_aspect->aspect_list.empty())
                {
                    HttpServerTask *server_task = task_of(resp);
                    server_task->add_callback([req, resp, global_aspect](HttpTask *)
                    {
                        for(auto it = global_aspect->aspect_list.rbegin(); it != global_aspect->aspect_list.rend(); ++it)
                        {
                            (*it)->after(req, resp);
                        }
                    });
                }
                return nullptr;
            };

    router_.handle(route, -1, wrap_handler, verb);
}

void BluePrint::ROUTE(const std::string &route, int compute_queue_id, const SeriesHandler &handler, Verb verb)
{
    WrapHandler wrap_handler =
            [handler, compute_queue_id](HttpReq *req,
                                        HttpResp *resp,
                                        SeriesWork *series) -> WFGoTask *
            {
                GlobalAspect *global_aspect = GlobalAspect::get_instance();
                for(auto asp : global_aspect->aspect_list)
                {
                    auto ret = asp->before(req, resp);
                    if(!ret) return nullptr;
                }
                WFGoTask *go_task = WFTaskFactory::create_go_task(
                        "wfrest" + std::to_string(compute_queue_id),
                        handler,
                        req,
                        resp,
                        series);
                if(!global_aspect->aspect_list.empty())
                {
                    HttpServerTask *server_task = task_of(resp);
                    server_task->add_callback([req, resp, global_aspect](HttpTask *)
                    {
                        for(auto it = global_aspect->aspect_list.rbegin(); it != global_aspect->aspect_list.rend(); ++it)
                        {
                            (*it)->after(req, resp);
                        }
                    });
                }
                return go_task;
            };

    router_.handle(route, compute_queue_id, wrap_handler, verb);
}

void BluePrint::ROUTE(const std::string &route, const SeriesHandler &handler, const std::vector<std::string> &methods)
{
    for(const auto &method : methods)
    {
        this->ROUTE(route, handler, str_to_verb(method));
    }
}

void BluePrint::ROUTE(const std::string &route, int compute_queue_id,
            const SeriesHandler &handler, const std::vector<std::string> &methods)
{
    for(const auto &method : methods)
    {
        this->ROUTE(route, compute_queue_id, handler, str_to_verb(method));
    }
}

void BluePrint::GET(const std::string &route, const SeriesHandler &handler)
{
    this->ROUTE(route, -1, handler, Verb::GET);
}

void BluePrint::GET(const std::string &route, int compute_queue_id, const SeriesHandler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::GET);
}

void BluePrint::POST(const std::string &route, const SeriesHandler &handler)
{
    this->ROUTE(route, -1, handler, Verb::POST);
}

void BluePrint::POST(const std::string &route, int compute_queue_id, const SeriesHandler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::POST);
}

void BluePrint::DELETE(const std::string &route, const SeriesHandler &handler)
{
    this->ROUTE(route, -1, handler, Verb::DELETE);
}

void BluePrint::DELETE(const std::string &route, int compute_queue_id, const SeriesHandler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::DELETE);
}

void BluePrint::PATCH(const std::string &route, const SeriesHandler &handler)
{
    this->ROUTE(route, -1, handler, Verb::PATCH);
}

void BluePrint::PATCH(const std::string &route, int compute_queue_id, const SeriesHandler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::PATCH);
}

void BluePrint::PUT(const std::string &route, const SeriesHandler &handler)
{
    this->ROUTE(route, -1, handler, Verb::PUT);
}

void BluePrint::PUT(const std::string &route, int compute_queue_id, const SeriesHandler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::PUT);
}

void BluePrint::HEAD(const std::string &route, const SeriesHandler &handler)
{
    this->ROUTE(route, -1, handler, Verb::HEAD);
}

void BluePrint::HEAD(const std::string &route, int compute_queue_id, const SeriesHandler &handler)
{
    this->ROUTE(route, compute_queue_id, handler, Verb::HEAD);
}

void BluePrint::add_blueprint(const BluePrint &bp, const std::string &url_prefix)
{
    bp.router_.routes_map_.all_routes([this, &url_prefix]
    (const std::string &sub_prefix, VerbHandler verb_handler)
    {
        std::string path = url_prefix;
        if (!path.empty() && path.back() == '/')
        {
            path.pop_back();
        }
        if (!sub_prefix.empty())
        {
            if (sub_prefix.front() != '/')
            {
                path += '/';
            }
            path += sub_prefix;
        }
        if (!path.empty() && path.back() == '/')
        {
            path.pop_back();
        }
        if (path.empty())
        {
            path = "/";
        }

        std::vector<Verb> verb_list;
        for(auto &vh_item : verb_handler.verb_handler_map)
        {
            verb_list.push_back(vh_item.first);
        }
        std::pair<Router::RouteVerbIter, bool> rv_pair = this->router_.add_route(verb_list, path.c_str());

        VerbHandler &vh = this->router_.routes_map_.find_or_create(rv_pair.first->route.c_str());
        vh = verb_handler;
        vh.path = rv_pair.first->route;
    });
}

