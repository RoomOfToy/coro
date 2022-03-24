#pragma once

#include <coroutine>
#include <exception>
#include <utility>
#include <source_location>

#include <fmt/core.h>

namespace coro
{
    template<typename Ret = void>
    struct task;

    namespace detail
    {
        struct promise_base
        {
            struct final_awaiter
            {
                bool await_ready() noexcept { return false; }
                void await_resume() noexcept { }

                template<typename promise_type>
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> coroutine) noexcept
                {
                    auto continuation = coroutine.promise().m_continuation;
                    if (continuation != nullptr)
                        return continuation;
                    else
                        return std::noop_coroutine();
                }
            };

            std::suspend_always initial_suspend() { return { }; }
            final_awaiter final_suspend() noexcept { return { }; }
            void unhandled_exception() { m_exception_ptr = std::current_exception(); }

            void set_continuation(std::coroutine_handle<> continuation) noexcept { m_continuation = continuation; }

            // FIXME: awaitable concept?
            template<typename A>
            decltype(auto) await_transform(A&& awaiter, // for collecting source_location info
                                           std::source_location loc = std::source_location::current()) {
                m_frame_info = loc;
                return std::forward<A>(awaiter);
            }

            std::source_location const& get_frame_info() const { return m_frame_info; }
            void dump_backtrace(size_t depth = 0) const
            {
                auto frame_name = fmt::format("{} at {}:{}", m_frame_info.function_name(), m_frame_info.file_name(), m_frame_info.line());
                fmt::print("[{}] {}\n", depth, frame_name);
                if (auto p = std::coroutine_handle<promise_base>::from_address(m_continuation.address())) { p.promise().dump_backtrace(depth + 1); }
                else fmt::print("\n"); 
            }

        protected:
            std::coroutine_handle<> m_continuation{ nullptr };
            std::exception_ptr m_exception_ptr{ };
            std::source_location m_frame_info;
        };

        template<typename Ret>
        struct promise final : public promise_base
        {
            using handle_type = std::coroutine_handle<promise<Ret>>;
            using task_type = task<Ret>;

            task_type get_return_object() noexcept;

            void return_value(Ret value) { m_ret_value = std::move(value); }

            Ret const& result() const &
            {
                if (m_exception_ptr)
                    std::rethrow_exception(m_exception_ptr);
                return m_ret_value;
            }

            Ret&& result() &&
            {
                if (m_exception_ptr)
                    std::rethrow_exception(m_exception_ptr);
                return std::move(m_ret_value);
            }

        private:
            Ret m_ret_value;
        };

        template<>
        struct promise<void> : public promise_base
        {
            using handle_type = std::coroutine_handle<promise<void>>;
            using task_type = task<void>;

            task_type get_return_object() noexcept;
            
            void return_void() noexcept { }

            void result()
            {
                if (m_exception_ptr)
                    std::rethrow_exception(m_exception_ptr);
            }
        };
    }

    template<typename Ret>
    struct task
    {
        using task_type = task<Ret>;
        using promise_type = detail::promise<Ret>;
        using handle_type = std::coroutine_handle<promise_type>;

        task() = default;
        task(handle_type handle) : m_coroutine(handle) { }
        ~task() { if (m_coroutine != nullptr) m_coroutine.destroy(); }

        task(task const&) = delete;
        task& operator=(task const&) = delete;
        task(task&& other) noexcept : m_coroutine(std::exchange(other.m_coroutine, nullptr)) { }
        task& operator=(task&& other) noexcept
        {
            if (std::addressof(other) != this)
            {
                if (m_coroutine != nullptr)
                    m_coroutine.destroy();
                m_coroutine = std::exchange(other.m_coroutine, nullptr);
            }
            return *this;
        }

        promise_type& promise() & { return m_coroutine.promise(); }
        promise_type const& promise() const & { return m_coroutine.promise(); }
        promise_type&& promise() && { return std::move(m_coroutine.promise()); }

        handle_type handle() { return m_coroutine; }

        bool is_done() const noexcept { return m_coroutine == nullptr || m_coroutine.done(); }

        bool resume()
        {
            if (m_coroutine == nullptr) return false;
            if (!m_coroutine.done()) m_coroutine.resume();
            return !m_coroutine.done();
        }

        bool destroy()
        {
            if (m_coroutine == nullptr) return false;
            m_coroutine.destroy();
            m_coroutine = nullptr;
            return true;
        }

        struct awaiter_base
        {
            std::coroutine_handle<promise_type> m_coroutine{ nullptr };

            awaiter_base(handle_type coroutine) noexcept : m_coroutine(coroutine) { }
            bool await_ready() const noexcept { return !m_coroutine || m_coroutine.done(); }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept
            {
                m_coroutine.promise().set_continuation(awaiting_coroutine);
                return m_coroutine;
            }
        };

        auto operator co_await() const & noexcept
        {
            struct awaiter : public awaiter_base
            {
                decltype(auto) await_resume()
                {
                    if constexpr (std::is_same_v<void, Ret>)
                    {
                        this->m_coroutine.promise().result();
                        return;
                    }
                    else
                        return this->m_coroutine.promise().result();
                }
            };

            return awaiter{ m_coroutine };
        }

        auto operator co_await() const && noexcept
        {
            struct awaiter : public awaiter_base
            {
                decltype(auto) await_resume()
                {
                    if constexpr (std::is_same_v<void, Ret>)
                    {
                        this->m_coroutine.promise().result();
                        return;
                    }
                    else
                        return std::move(this->m_coroutine.promise()).result();
                }
            };

            return awaiter{ m_coroutine };
        }

    private:
        handle_type m_coroutine{ nullptr };
    };

    namespace detail
    {
        template<typename Ret>
        inline task<Ret> promise<Ret>::get_return_object() noexcept { return task<Ret>(handle_type::from_promise(*this)); }

        inline task<> promise<void>::get_return_object() noexcept { return task<>(handle_type::from_promise(*this)); }

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
    }

    auto dump_callstack() -> detail::CallStackAwaiter { return {}; }
}