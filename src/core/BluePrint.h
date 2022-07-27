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
    void ROUTE(const std::string &route, const Handler &handler, Verb verb);

    void ROUTE(const std::string &route, int compute_queue_id, const Handler &handler, Verb verb);

    void ROUTE(const std::string &route, const Handler &handler, const std::vector<std::string> &methods);

    void ROUTE(const std::string &route, int compute_queue_id, 
                const Handler &handler, const std::vector<std::string> &methods);

    void GET(const std::string &route, const Handler &handler);

    void GET(const std::string &route, int compute_queue_id, const Handler &handler);

    void POST(const std::string &route, const Handler &handler);

    void POST(const std::string &route, int compute_queue_id, const Handler &handler);

    void DELETE(const std::string &route, const Handler &handler);

    void DELETE(const std::string &route, int compute_queue_id, const Handler &handler);

    void PATCH(const std::string &route, const Handler &handler);

    void PATCH(const std::string &route, int compute_queue_id, const Handler &handler);

    void PUT(const std::string &route, const Handler &handler);

    void PUT(const std::string &route, int compute_queue_id, const Handler &handler);
    
    void HEAD(const std::string &route, const Handler &handler);

    void HEAD(const std::string &route, int compute_queue_id, const Handler &handler);

public:
    template<typename... AP>
    void ROUTE(const std::string &route, const Handler &handler, 
                Verb verb, const AP &... ap);

    template<typename... AP>
    void ROUTE(const std::string &route, int compute_queue_id, 
                const Handler &handler, Verb verb, const AP &... ap);

    template<typename... AP>
    void ROUTE(const std::string &route, const Handler &handler, 
                const std::vector<std::string> &methods, const AP &... ap);

    template<typename... AP>
    void ROUTE(const std::string &route, int compute_queue_id, 
                const Handler &handler, 
                const std::vector<std::string> &methods, const AP &... ap);

    template<typename... AP>
    void GET(const std::string &route, const Handler &handler, const AP &... ap);

    template<typename... AP>
    void GET(const std::string &route, int compute_queue_id,
             const Handler &handler, const AP &... ap);

    template<typename... AP>
    void POST(const std::string &route, const Handler &handler, const AP &... ap);

    template<typename... AP>
    void POST(const std::string &route, int compute_queue_id,
              const Handler &handler, const AP &... ap);

    template<typename... AP>
    void DELETE(const std::string &route, const Handler &handler, const AP &... ap);

    template<typename... AP>
    void DELETE(const std::string &route, int compute_queue_id,
             const Handler &handler, const AP &... ap);

    template<typename... AP>
    void PATCH(const std::string &route, const Handler &handler, const AP &... ap);

    template<typename... AP>
    void PATCH(const std::string &route, int compute_queue_id,
             const Handler &handler, const AP &... ap);

    template<typename... AP>
    void PUT(const std::string &route, const Handler &handler, const AP &... ap);

    template<typename... AP>
    void PUT(const std::string &route, int compute_queue_id,
             const Handler &handler, const AP &... ap);

    template<typename... AP>
    void HEAD(const std::string &route, const Handler &handler, const AP &... ap);

    template<typename... AP>
    void HEAD(const std::string &route, int compute_queue_id,
             const Handler &handler, const AP &... ap);

public:
    void ROUTE(const std::string &route, const SeriesHandler &handler, Verb verb);

    void ROUTE(const std::string &route, int compute_queue_id, const SeriesHandler &handler, Verb verb);

    void ROUTE(const std::string &route, const SeriesHandler &handler, const std::vector<std::string> &methods);

    void ROUTE(const std::string &route, int compute_queue_id, 
                const SeriesHandler &handler, const std::vector<std::string> &methods);
    
    void GET(const std::string &route, const SeriesHandler &handler);

    void GET(const std::string &route, int compute_queue_id, const SeriesHandler &handler);

    void POST(const std::string &route, const SeriesHandler &handler);

    void POST(const std::string &route, int compute_queue_id, const SeriesHandler &handler);  

    void DELETE(const std::string &route, const SeriesHandler &handler);

    void DELETE(const std::string &route, int compute_queue_id, const SeriesHandler &handler);  

    void PATCH(const std::string &route, const SeriesHandler &handler);

    void PATCH(const std::string &route, int compute_queue_id, const SeriesHandler &handler);  

    void PUT(const std::string &route, const SeriesHandler &handler);

    void PUT(const std::string &route, int compute_queue_id, const SeriesHandler &handler);  

    void HEAD(const std::string &route, const SeriesHandler &handler);

    void HEAD(const std::string &route, int compute_queue_id, const SeriesHandler &handler);  

public:
    template<typename... AP>
    void ROUTE(const std::string &route, const SeriesHandler &handler, 
                Verb verb, const AP &... ap);

    template<typename... AP>
    void ROUTE(const std::string &route, int compute_queue_id, 
                const SeriesHandler &handler, Verb verb, const AP &... ap);

    template<typename... AP>
    void ROUTE(const std::string &route, const SeriesHandler &handler, 
                const std::vector<std::string> &methods, const AP &... ap);

    template<typename... AP>
    void ROUTE(const std::string &route, int compute_queue_id, 
                const SeriesHandler &handler, 
                const std::vector<std::string> &methods, const AP &... ap);
                
    template<typename... AP>
    void GET(const std::string &route, const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void GET(const std::string &route, int compute_queue_id,
             const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void POST(const std::string &route, const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void POST(const std::string &route, int compute_queue_id,
              const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void DELETE(const std::string &route, const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void DELETE(const std::string &route, int compute_queue_id,
             const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void PATCH(const std::string &route, const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void PATCH(const std::string &route, int compute_queue_id,
             const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void PUT(const std::string &route, const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void PUT(const std::string &route, int compute_queue_id,
             const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void HEAD(const std::string &route, const SeriesHandler &handler, const AP &... ap);

    template<typename... AP>
    void HEAD(const std::string &route, int compute_queue_id,
             const SeriesHandler &handler, const AP &... ap);

public:
    const Router &router() const
    { return router_; }

    void add_blueprint(const BluePrint &bp, const std::string &url_prefix);

    void print_node_arch() { router_.print_node_arch(); }  // for test
private:
    Router router_;    // ptr for hiding internel class
    friend class HttpServer;
};


} // namespace wfrest

#include "BluePrint.inl"

#endif // WFREST_BLUEPRINT_H_