#ifndef WFREST_BLUEPRINT_H_
#define WFREST_BLUEPRINT_H_

#include <functional>
#include <utility>
#include "Noncopyable.h"
#include "Aspect.h"
#include "AopUtil.h"

// todo : hide
#include "Router.h"
#include "HttpServerTask.h" 

class SeriesWork;
namespace wfrest
{
using Handler = std::function<void(const HttpReq *, HttpResp *)>;
using SeriesHandler = std::function<void(const HttpReq *, HttpResp *, SeriesWork *)>;

class BluePrint : public Noncopyable
{
public:
    // reserve basic interface
    void ROUTE(const char *route, const Handler &handler, Verb verb);

    void ROUTE(const char *route, int compute_queue_id, const Handler &handler, Verb verb);

    void ROUTE(const char *route, const Handler &handler, const std::vector<std::string> &methods);

    void ROUTE(const char *route, int compute_queue_id, 
                const Handler &handler, const std::vector<std::string> &methods);

    void GET(const char *route, const Handler &handler);

    void GET(const char *route, int compute_queue_id, const Handler &handler);

    void POST(const char *route, const Handler &handler);

    void POST(const char *route, int compute_queue_id, const Handler &handler);

    void DELETE(const char *route, const Handler &handler);

    void DELETE(const char *route, int compute_queue_id, const Handler &handler);

    void PATCH(const char *route, const Handler &handler);

    void PATCH(const char *route, int compute_queue_id, const Handler &handler);

    void PUT(const char *route, const Handler &handler);

    void PUT(const char *route, int compute_queue_id, const Handler &handler);
    
    void HEAD(const char *route, const Handler &handler);

    void HEAD(const char *route, int compute_queue_id, const Handler &handler);

public:
    template<typename... AP>
    void ROUTE(const char *route, const Handler &handler, 
                Verb verb, const AP &... ap);

    template<typename... AP>
    void ROUTE(const char *route, int compute_queue_id, 
                const Handler &handler, Verb verb, const AP &... ap);

    template<typename... AP>
    void ROUTE(const char *route, const Handler &handler, 
                const std::vector<std::string> &methods, const AP &... ap);

    template<typename... AP>
    void ROUTE(const char *route, int compute_queue_id, 
                const Handler &handler, 
                const std::vector<std::string> &methods, const AP &... ap);

    template<typename... AP>
    void GET(const char *route, const Handler &handler, const AP &... ap);

    template<typename... AP>
    void GET(const char *route, int compute_queue_id,
             const Handler &handler, const AP &... ap);

    template<typename... AP>
    void POST(const char *route, const Handler &handler, const AP &... ap);

    template<typename... AP>
    void POST(const char *route, int compute_queue_id,
              const Handler &handler, const AP &... ap);

    template<typename... AP>
    void DELETE(const char *route, const Handler &handler, const AP &... ap);

    template<typename... AP>
    void DELETE(const char *route, int compute_queue_id,
             const Handler &handler, const AP &... ap);

    template<typename... AP>
    void PATCH(const char *route, const Handler &handler, const AP &... ap);

    template<typename... AP>
    void PATCH(const char *route, int compute_queue_id,
             const Handler &handler, const AP &... ap);

    template<typename... AP>
    void PUT(const char *route, const Handler &handler, const AP &... ap);

    template<typename... AP>
    void PUT(const char *route, int compute_queue_id,
             const Handler &handler, const AP &... ap);

    template<typename... AP>
    void HEAD(const char *route, const Handler &handler, const AP &... ap);

    template<typename... AP>
    void HEAD(const char *route, int compute_queue_id,
             const Handler &handler, const AP &... ap);

public:
    void ROUTE(const char *route, const SeriesHandler &handler, Verb verb);

    void ROUTE(const char *route, int compute_queue_id, const SeriesHandler &handler, Verb verb);

    void ROUTE(const char *route, const SeriesHandler &handler, const std::vector<std::string> &methods);

    void ROUTE(const char *route, int compute_queue_id, 
                const SeriesHandler &handler, const std::vector<std::string> &methods);
    
    void GET(const char *route, const SeriesHandler &handler);

    void GET(const char *route, int compute_queue_id, const SeriesHandler &handler);

    void POST(const char *route, const SeriesHandler &handler);

    void POST(const char *route, int compute_queue_id, const SeriesHandler &handler);  

    void DELETE(const char *route, const SeriesHandler &handler);

    void DELETE(const char *route, int compute_queue_id, const SeriesHandler &handler);  

    void PATCH(const char *route, const SeriesHandler &handler);

    void PATCH(const char *route, int compute_queue_id, const SeriesHandler &handler);  

    void PUT(const char *route, const SeriesHandler &handler);

    void PUT(const char *route, int compute_queue_id, const SeriesHandler &handler);  

    void HEAD(const char *route, const SeriesHandler &handler);

    void HEAD(const char *route, int compute_queue_id, const SeriesHandler &handler);  

public:
    template<typename... AP>
    void ROUTE(const char *route, const SeriesHandler &handler, 
                Verb verb, const AP &... ap);

    template<typename... AP>
    void ROUTE(const char *route, int compute_queue_id, 
                const SeriesHandler &handler, Verb verb, const AP &... ap);

    template<typename... AP>
    void ROUTE(const char *route, const SeriesHandler &handler, 
                const std::vector<std::string> &methods, const AP &... ap);

    template<typename... AP>
    void ROUTE(const char *route, int compute_queue_id, 
                const SeriesHandler &handler, 
                const std::vector<std::string> &methods, const AP &... ap);
                
    template<typename... AP>
    void GET(const char *route, const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void GET(const char *route, int compute_queue_id,
             const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void POST(const char *route, const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void POST(const char *route, int compute_queue_id,
              const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void DELETE(const char *route, const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void DELETE(const char *route, int compute_queue_id,
             const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void PATCH(const char *route, const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void PATCH(const char *route, int compute_queue_id,
             const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void PUT(const char *route, const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void PUT(const char *route, int compute_queue_id,
             const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void HEAD(const char *route, const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void HEAD(const char *route, int compute_queue_id,
             const SeriesHandler &handler, const AP &... ap);

public:
    const Router &router() const
    { return router_; }

    void add_blueprint(const BluePrint &bp, const std::string &url_prefix);

private:
    Router router_;    // ptr for hiding internel class
    friend class HttpServer;
};


} // namespace wfrest

#include "wfrest/BluePrint.inl"

#endif // WFREST_BLUEPRINT_H_