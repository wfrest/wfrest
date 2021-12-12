#ifndef WFREST_BLUEPRINT_H_
#define WFREST_BLUEPRINT_H_

#include <functional>

class SeriesWork;

namespace wfrest
{
class Router;
class HttpReq;
class HttpResp;

using Handler = std::function<void(HttpReq * , HttpResp *)>;
using SeriesHandler = std::function<void(HttpReq * , HttpResp *, SeriesWork *)>;

class BluePrint
{
public:
    void GET(const char *route, const Handler &handler);

    void GET(const char *route, int compute_queue_id, const Handler &handler);

    void POST(const char *route, const Handler &handler);

    void POST(const char *route, int compute_queue_id, const Handler &handler);

public:
    void GET(const char *route, const SeriesHandler &handler);

    void GET(const char *route, int compute_queue_id, const SeriesHandler &handler);

    void POST(const char *route, const SeriesHandler &handler);

    void POST(const char *route, int compute_queue_id, const SeriesHandler &handler);

public:
    const Router &router() const;

    void add_blueprint(const BluePrint &bp, const std::string &url_prefix);

public:
    BluePrint();

    ~BluePrint();
    
private: 
    Router *router_;    // ptr for hiding internel class
};

} // namespace wfrest



#endif // WFREST_BLUEPRINT_H_