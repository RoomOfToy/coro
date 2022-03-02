#include <coroutine>
#include <source_location>
#include <utility>

#include <fmt/core.h>

#include "coro/concepts/awaitable.h"

template<typename T>
struct task
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    task(handle_type h) : coro(h) { }
    ~task() { if (coro) coro.destroy(); }

    task(task const&) = delete;
    task& operator=(task const&) = delete;
    task(task&&) = delete;
    task& operator=(task&&) = delete;

    struct promise_type
    {
        T result;
        std::coroutine_handle<> previous;
        std::source_location frame_info;

        auto get_return_object() { return task(handle_type::from_promise(*this)); }
        std::suspend_always initial_suspend() { return {}; }

        struct final_awaiter
        {
            bool await_ready() noexcept { return false; }
            void await_resume() noexcept { }
            std::coroutine_handle<> await_suspend(handle_type h) noexcept
            {
                // final_awaiter::await_suspend is called when the execution of the
                // current coroutine (referred to by 'h') is about to finish.
                // If the current coroutine was resumed by another coroutine via
                // co_await get_task(), a handle to that coroutine has been stored
                // as h.promise().previous. In that case, return the handle to resume
                // the previous coroutine.
                // Otherwise, return noop_coroutine(), whose resumption does nothing.

                auto pre = h.promise().previous;
                if (pre)
                    return pre;
                else
                    return std::noop_coroutine();
            }
        };

        final_awaiter final_suspend() noexcept { return {}; }
        void unhandled_exception() { throw; }
        void return_value(T value) { result = std::move(value); }

        template<coro::concepts::Awaitable A>
        decltype(auto) await_transform(A&& awaiter, // for collecting source_location info
                                       std::source_location loc = std::source_location::current()) {
            frame_info = loc;
            return std::forward<A>(awaiter);
        }

        std::source_location const& get_frame_info() const { return frame_info; }
        void dump_backtrace(size_t depth = 0) const
        {
            auto frame_name = fmt::format("{} at {}:{}", frame_info.function_name(), frame_info.file_name(), frame_info.line());
            fmt::print("[{}] {}\n", depth, frame_name);
            // TODO: safe?
            if (auto p = handle_type::from_address(previous.address())) { p.promise().dump_backtrace(depth + 1); }
            else fmt::print("\n"); 
        }
    };

    struct awaiter
    {
        handle_type coro;

        bool await_ready() { return false; }
        T await_resume() { return std::move(coro.promise().result); }
        auto await_suspend(std::coroutine_handle<> h)
        {
            coro.promise().previous = h;
            return coro;
        }
    };

    awaiter operator co_await() { return awaiter{ coro }; }

    T operator()()
    {
        coro.resume();
        return std::move(coro.promise().result);
    }

private:
    handle_type coro;
};

struct CallStackAwaiter
{
    bool await_ready() const noexcept { return false; }
    void await_resume() const noexcept { }

    template<typename Promise>
    bool await_suspend(std::coroutine_handle<Promise> caller) const noexcept
    {
        caller.promise().dump_backtrace();
        return false;
    }
};

auto dump_callstack() -> CallStackAwaiter { return {}; }

task<int> factorial(int n)
{
    if (n <= 1)
    {
        co_await dump_callstack();
        co_return 1;
    }
    co_return (co_await factorial(n - 1)) * n;
}

int main()
{
    fmt::print("{}\n", factorial(5)());

    return 0;
}