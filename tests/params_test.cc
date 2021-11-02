//
// Created by Chanchan on 11/2/21.
//

#include <iostream>

struct S {
    template <typename Fn>
    auto update(Fn &&fn_)
    -> decltype(fn_(0, 0), void())
    {
        // ...
        fn_(0, 0);
        // ...
    }

    template <typename Fn>
    auto update(Fn &&fn_)
    -> decltype(fn_(0), void())
    {
        update([&fn_](auto a, auto)
        {
            fn_(a);
        });
    }

    template <typename Fn>
    auto update(Fn &&fn_)
    -> decltype(fn_(), void())
    { update([&fn_](auto, auto) { fn_(); }); }
};

int main() {
    S s;
    s.update([](auto, auto) {});
    s.update([](auto) {});
    s.update([]() {});
}