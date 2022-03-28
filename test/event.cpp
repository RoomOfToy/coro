#include "coro/event.h"
#include "coro/task.h"
#include <thread>
#include <chrono>
#include <fmt/core.h>

#define RequireFalse(x) fmt::print("Require False: {}\n", x)
#define RequireTrue(x) fmt::print("Require True: {}\n", x)

using namespace coro;

void trigger(event& e) { e.set(); }
task<int> waiter(event const& e) { co_await e; co_return 1; }

int main()
{
    {
        event e{};
        RequireFalse(e.is_set());

        auto t = [&e]() -> task<int> {
            auto start = std::chrono::high_resolution_clock::now();
            co_await e;
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            fmt::print("time: {}\n", elapsed.count());
            co_return 1;
        }();
        t.resume();
        RequireFalse(t.is_done());

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
        e.set();

        RequireTrue(e.is_set());
        RequireTrue(t.is_done());
        RequireTrue(t.promise().result() == 1);
    }

    {
        event e{};

        auto t1 = waiter(e);
        auto t2 = waiter(e);
        auto t3 = waiter(e);
        t1.resume();
        t2.resume();
        t3.resume();
        RequireFalse(t1.is_done());
        RequireFalse(t2.is_done());
        RequireFalse(t3.is_done());

        trigger(e);

        RequireTrue(t1.promise().result() == 1);
        RequireTrue(t2.promise().result() == 1);
        RequireTrue(t3.promise().result() == 1);
    }

    {
        event e{};

        auto t1 = waiter(e);
        t1.resume();
        trigger(e);
        RequireTrue(t1.promise().result() == 1);

        e.reset();

        auto t2 = waiter(e);
        t2.resume();
        trigger(e);
        RequireTrue(t2.promise().result() == 1);
    }

    return 0;
}