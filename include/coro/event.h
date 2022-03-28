#pragma once

#include <coroutine>
#include <atomic>

namespace coro
{
    class event
    {
    public:
        event() = default;
        event(event const&) = delete;
        event(event&&) = delete;
        event& operator=(event const&) = delete;
        event& operator=(event&&) = delete;

        void set() noexcept;
        void reset() noexcept;
        bool is_set() const noexcept;

        class awaiter;
        awaiter operator co_await() const noexcept;

    private:
        friend class awaiter;

        /**
         * nullptr: not set
         * awaiter*: awaiters waiting on this event
         * this: set and all awaiters are resumed
         */
        mutable std::atomic<void*> suspended_awaiter{ nullptr };
    };

    class event::awaiter
    {
    public:
        awaiter(event const& e) : m_event(e) { }

        bool await_ready() const noexcept { return m_event.is_set(); }

        bool await_suspend(std::coroutine_handle<> coroutine) noexcept
        {
            m_coroutine = coroutine;
            void const* set = &m_event;
            void* old = m_event.suspended_awaiter.load(std::memory_order_acquire);
            // stack push
            do
            {
                if (old == set) return false;  // already set
                m_next = static_cast<awaiter*>(old);  // prepend awaiter
            }
            // update old pointer if other threads write to suspended_awaiter, `this` is the new node
            while (!m_event.suspended_awaiter.compare_exchange_weak(old, this, std::memory_order_release, std::memory_order_acquire));
            
            return true;
        }

        void await_resume() noexcept { }

    private:
        friend event;

        event const& m_event;
        std::coroutine_handle<> m_coroutine { nullptr };
        awaiter* m_next{ nullptr };  // linked list as stack
    };

    void event::set() noexcept
    {
        auto* old = suspended_awaiter.exchange(this, std::memory_order_acq_rel);
        if (old != this)
        {
            auto* waiter = static_cast<awaiter*>(old);
            while (waiter != nullptr)
            {
                auto* next = waiter->m_next;
                waiter->m_coroutine.resume();
                waiter = next;
            }
        }
    }

    void event::reset() noexcept
    {
        void* old = this;
        suspended_awaiter.compare_exchange_strong(old, nullptr, std::memory_order_acquire);
    }

    bool event::is_set() const noexcept
    {
        return suspended_awaiter.load(std::memory_order_acquire) == this;
    }

    event::awaiter event::operator co_await() const noexcept
    {
        return awaiter{ *this };
    }
}