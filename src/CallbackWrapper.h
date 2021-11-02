//
// Created by Chanchan on 11/2/21.
//

#ifndef _CALLBACKWRAPPER_H_
#define _CALLBACKWRAPPER_H_

namespace wfrest
{
    template<typename Func>
    struct Wrapped
    {

        std::function<void(HttpReq *, HttpResp *)> handler_;

        template <typename ... Args>
        void set(Func f,
             typename std::enable_if<
            !std::is_same<typename std::tuple_element<0, std::tuple < Args..., void>>::type, HttpReq *>::value,
            int>::type = 0);
        {
            handler_ =
            (
                [f](HttpReq *, HttpResp *)
                {}
            };);
        }

        template <typename ... Args>
        void set(Func f,
             typename std::enable_if<
                std::is_same<typename std::tuple_element<0, std::tuple < Args..., void>>::type, HttpReq *>::value &&
                std::is_same<typename std::tuple_element<1, std::tuple < Args..., void, void>>::type, HttpResp *>::value,
                int>::type = 0)
        {
            handler_ = std::move(f);
        }

        template <typename ... Args>
        void set(Func f,
             typename std::enable_if<
            std::is_same<typename std::tuple_element<0, std::tuple < Args..., void>>::type, HttpRequest *>::value &&
            !std::is_same<typename std::tuple_element<1, std::tuple < Args..., void, void>>::type, response&>::value,
            int>::type = 0)
        {
            handler_ = std::move(f);
        }


    };

    template<typename Func>
    std::function<void(HttpReq * , HttpResp * )>
    wrap(Func f)
    {
        auto ret = Wrapped<Func>();
        ret.set(std::move(f));
        return ret;
    }

}   // wfrest

#endif //_CALLBACKWRAPPER_H_
