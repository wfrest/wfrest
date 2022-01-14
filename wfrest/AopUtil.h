#ifndef WFREST_AOPUTIL_H_
#define WFREST_AOPUTIL_H_

#include <cstddef>
#include <tuple>
#include "wfrest/Aspect.h"

namespace wfrest
{

template<std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
for_each(std::tuple<Tp...> &, FuncT)
{}

template<std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), void>::type
for_each(std::tuple<Tp...> &tp, FuncT func)
{
    func(std::get<I>(tp));
    for_each<I + 1, FuncT, Tp...>(tp, func);
}

template<std::size_t I = 0, typename FuncT, typename Server, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
for_each(std::tuple<Tp...> &, FuncT, Server *)
{}

template<std::size_t I = 0, typename FuncT, typename Server, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), void>::type
for_each(std::tuple<Tp...> &tp, FuncT func, Server *server)
{
    func(server, std::get<I>(tp));
    for_each<I + 1, FuncT, Server, Tp...>(tp, func, server);
}

class HttpReq;
class HttpResp;

template<typename Tuple>
inline bool aop_before(const HttpReq *req, HttpResp *resp, Tuple &tp)
{
    bool ret = true;
    for_each(
            tp,
            [&ret, req, resp](Aspect &item)
            {
                if (!ret)
                    return;
                ret = item.before(req, resp);
            });
    return ret;
}

template<typename Tuple>
inline bool aop_after(const HttpReq *req, HttpResp *resp, Tuple &tp)
{
    bool ret = true;
    for_each(
            tp,
            [&ret, req, resp](Aspect &item)
            {
                if (!ret)
                    return;
                ret = item.after(req, resp);
            });
    return ret;
}

} // namespace wfrest


#endif // WFREST_AOPUTIL_H_