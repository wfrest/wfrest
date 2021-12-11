#ifndef WFREST_BLUEPRINT_H_
#define WFREST_BLUEPRINT_H_

#include "wfrest/Router.h"

namespace wfrest
{
class BluePrint
{
public:
    void GET(const char *route, const CustomHandler &handler);

    void GET(const char *route, int compute_queue_id, const CustomHandler &handler);

    void POST(const char *route, const CustomHandler &handler);

    void POST(const char *route, int compute_queue_id, const CustomHandler &handler);

public:
    void GET(const char *route, const SeriesHandler &handler);

    void GET(const char *route, int compute_queue_id, const SeriesHandler &handler);

    void POST(const char *route, const SeriesHandler &handler);

    void POST(const char *route, int compute_queue_id, const SeriesHandler &handler);

public:
    const Router &router() const
    { return router_; }

    void add_blueprint(const BluePrint &bp, const std::string &url_prefix);

private:
    Router router_;
};

} // namespace wfrest



#endif // WFREST_BLUEPRINT_H_