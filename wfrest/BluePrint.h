#ifndef WFREST_BLUEPRINT_H_
#define WFREST_BLUEPRINT_H_

#include <functional>
#include <utility>
#include "wfrest/Noncopyable.h"
#include "wfrest/AopUtil.h"
#include "wfrest/Router.h"
#include "wfrest/Logger.h"
#include "wfrest/Aop.h"

class SeriesWork;

namespace wfrest
{
class Router;
class HttpReq;
class HttpResp;

using Handler = std::function<void(HttpReq * , HttpResp *)>;
using SeriesHandler = std::function<void(HttpReq * , HttpResp *, SeriesWork *)>;

class BluePrint : public Noncopyable
{
public:
    template <typename... AP>
    void GET(const char *route, const Handler &handler, const AP &... ap)
    {
        WrapHandler wrap_handler = [handler, ap...](HttpReq *req, HttpResp *resp, SeriesWork *)
                                    { 
                                        std::tuple<AP...> tp(std::move(ap)...);
                                        bool r = do_ap_before(req, resp, tp);
                                        if(!r) 
                                        {
                                            LOG_DEBUG << "before wrong";
                                            return nullptr;
                                        }
                                        
                                        handler(req, resp); 
                                        return nullptr; 
                                    };

        router_->handle(route, -1, wrap_handler, Verb::GET);
    }

    // void GET(const char *route, const Handler &handler);

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

    void Register(std::function<void(BluePrint *)> register_func)
    { register_func(this); }
    
public:
    BluePrint();

    ~BluePrint();
    
private: 
    Router *router_;    // ptr for hiding internel class

};

} // namespace wfrest



#endif // WFREST_BLUEPRINT_H_