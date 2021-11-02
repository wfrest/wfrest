//
// Created by Chanchan on 11/2/21.
//

#include <iostream>

struct Req
{
    void test()
    { std::cout << "req" << std::endl; }

};

struct Resp
{
    void test()
    { std::cout << "resp" << std::endl; }
};

struct S
{
    template<typename Fn>
    auto update(Fn &&fn_)
    -> decltype(fn_(nullptr, nullptr), void())
    {
        Req req;
        Resp resp;
        fn_(&req, &resp);
        // ...
    }

    template<typename Fn>
    auto update(Fn &&fn_)
    -> decltype(fn_(0), void())
    {
        update([&fn_](Req *req, Resp *resp)
               {
                   fn_(req);
               });
    }

    template<typename Fn>
    auto update(Fn &&fn_)
    -> decltype(fn_(), void())
    {
        update([&fn_](Req *, Resp *resp)
               { fn_(); });
    }
};

int main()
{
    S s;
    s.update([](Req *req, Resp *resp)
             {
                 std::cout << "1" << std::endl;
                 req->test();
                 resp->test();
             });
    s.update([](Req *req)
             {
                 std::cout << "2" << std::endl;
                 req->test();
             });
    s.update([]()
             {
                 std::cout << "3" << std::endl;
             });
}