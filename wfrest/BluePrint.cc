#include "wfrest/BluePrint.h"
#include "wfrest/VerbHandler.h"

using namespace wfrest;

void BluePrint::GET(const char *route, const Handler &handler)
{
    router_.handle(route, -1, handler, nullptr, Verb::GET);
}

void BluePrint::GET(const char *route, const SeriesHandler &series_handler)
{
    router_.handle(route, -1, nullptr, series_handler, Verb::GET);
}

void BluePrint::GET(const char *route, int compute_queue_id, const Handler &handler)
{
    router_.handle(route, compute_queue_id, handler, nullptr, Verb::GET);
}

void BluePrint::GET(const char *route, int compute_queue_id, const SeriesHandler &series_handler)
{
    router_.handle(route, compute_queue_id, nullptr, series_handler, Verb::GET);
}

void BluePrint::POST(const char *route, const Handler &handler)
{
    router_.handle(route, -1, handler, nullptr, Verb::POST);
}

void BluePrint::POST(const char *route, const SeriesHandler &series_handler)
{
    router_.handle(route, -1, nullptr, series_handler, Verb::POST);
}

void BluePrint::POST(const char *route, int compute_queue_id, const Handler &handler)
{
    router_.handle(route, compute_queue_id, handler, nullptr, Verb::POST);
}

void BluePrint::POST(const char *route, int compute_queue_id, const SeriesHandler &series_handler)
{
    router_.handle(route, compute_queue_id, nullptr, series_handler, Verb::POST);
}

void BluePrint::add_blueprint(const BluePrint &bp, const std::string &url_prefix)
{
    bp.router_.routes_map_.all_routes([this, &url_prefix](
            const std::string& sub_prefix, VerbHandler verb_handler) {
        if (!url_prefix.empty() && url_prefix.back() == '/')
            verb_handler.path = url_prefix + sub_prefix;
        else
            verb_handler.path = url_prefix + "/" + sub_prefix;

        this->router_.routes_map_[verb_handler.path.c_str()] = verb_handler;
    });
}
