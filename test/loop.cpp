#include "coro/loop.h"
#include "coro/task.h"

using namespace coro;

int main()
{
    auto t = []() -> task<int> { co_await dump_callstack(); co_return 1; };

    auto sum = [&]() -> task<> {
        int sum = 0;
        for (int i = 0; i < 10; i++)
            sum += co_await t();
        
        fmt::print("sum == {}: {}\n", sum, sum == 10);
        co_return;
    }();

    //sum.promise().run();

    Loop loop;
    fmt::print("create loop\n");

    //loop.call(sum.promise());
    loop.call(sum);
    fmt::print("push task sum into loop queue\n");

    auto j = []() -> task<> { co_await dump_callstack(); }();
    loop.call(j);
    fmt::print("push task j into loop queue\n");

    auto k = []() -> task<int> { co_await dump_callstack(); co_return 2; }();
    using namespace std::chrono_literals;
    loop.call_after(std::chrono::duration(1s), k);
    fmt::print("push task k into loop queue\n");

    auto h = []() -> task<> { co_await dump_callstack(); }();
    using namespace std::chrono_literals;
    loop.call_after(std::chrono::duration(2s), h);
    fmt::print("push task h into loop queue\n");

    // add all tasks before this
    loop.run_until_complete();

    return 0;
}