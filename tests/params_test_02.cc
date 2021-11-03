//
// Created by Chanchan on 11/2/21.
//

//https://stackoverflow.com/questions/47113500/allow-multiple-signatures-for-lambda-callback-function-as-template-parameter/
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
//    https://coderedirect.com/questions/326487/what-does-the-void-in-auto-fparams-decltype-void-do
    template<typename Fn>
    auto update(Fn &&fn_)
    -> decltype(fn_(nullptr, nullptr), void())
    {
        Req req;
        Resp resp;
        fn_(&req, &resp);
    }

    template<typename Fn>
    auto update(Fn &&fn_)
    -> decltype(fn_(nullptr), void())
    {
        update([&fn_](Req *req, Resp *resp)
               {
                   fn_(resp);
               });
    }

    template<typename Fn>
    auto update(Fn &&fn_)
    -> decltype(fn_(), void())
    {
        update([&fn_](Req *req, Resp *resp)
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
    s.update([](Resp *resp)
             {
                 std::cout << "2" << std::endl;
             });
    s.update([]()
             {
                 std::cout << "3" << std::endl;
             });
}