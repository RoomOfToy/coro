#include "coro/task.h"
#include <string>
#include <stdexcept>

using namespace coro;

task<> task_void()
{
    co_await dump_callstack();
    co_return;
}

task<int> factorial(int n)
{
    if (n <= 1)
    {
        co_await dump_callstack();
        co_return 1;
    }
    co_return (co_await factorial(n - 1)) * n;
}

task<std::string> task_string()
{
    co_return "Hello";
}

task<> task_exception()
{
    throw std::runtime_error("error here");
    co_return;
}

task<> task_inside_task()
{
    auto t = []() -> task<int> { co_await dump_callstack(); co_return 1; };
    co_await t();
    co_await dump_callstack();
    co_return;
}

int main()
{
    auto t = task_void();
    t.resume();
    fmt::print("{}\n", t.is_done());

    auto t1 = factorial(5);
    t1.resume();
    fmt::print("{}\n", t1.is_done());
    fmt::print("{}\n", t1.promise().result());

    auto t2 = task_string();
    t2.resume();
    fmt::print("{}\n", t2.is_done());
    fmt::print("{}\n", t2.promise().result());

    auto t3 = task_exception();
    t3.resume();
    fmt::print("{}\n", t3.is_done());

    try
    {
        t3.promise().result();
    }
    catch(const std::exception& e)
    {
        fmt::print("{}\n", e.what());
    }

    auto t5 = task_inside_task();
    t5.resume();
    fmt::print("{}\n", t5.is_done());
    

    return 0;
}