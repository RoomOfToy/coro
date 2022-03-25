#pragma once

#include <coroutine>
#include <type_traits>
#include <exception>
#include <utility>
#include <iterator>

namespace coro
{
    template<typename T>
    struct generator
    {
        using value_type = std::remove_reference_t<T>;
        using pointer_type = value_type*;
        using reference_type = std::conditional_t<std::is_reference_v<T>, T, T&>;

        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        struct promise_type
        {
            generator<T> get_return_object() noexcept { return generator<T>{ handle_type::from_promise(*this) }; }

            std::suspend_always initial_suspend() const noexcept { return { }; }

            std::suspend_always final_suspend() const noexcept { return { }; }

            std::suspend_always yield_value(value_type&& value) noexcept
            {
                m_value = std::addressof(value);
                return { };
            }

            void unhandled_exception() { m_exception = std::current_exception(); }

            void return_void() noexcept { }

            reference_type value() const noexcept
            {
                return static_cast<reference_type>(*m_value);
            }

            void rethrow_if_exception()
            {
                if (m_exception)
                    std::rethrow_exception(m_exception);
            }

        private:
            pointer_type m_value{ nullptr };
            std::exception_ptr m_exception;
        };

        struct sentinel { };

        struct iterator
        {
            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;

            iterator() = default;
            explicit iterator(handle_type handle) noexcept : m_coroutine(handle) { }
            
            friend bool operator==(iterator const& it, sentinel) noexcept { return it.m_coroutine == nullptr || it.m_coroutine.done(); }

            friend bool operator==(sentinel s, iterator const& it) noexcept { return (it == s); }

            friend bool operator!=(iterator const& it, sentinel s) noexcept { return !(it == s); }

            friend bool operator!=(sentinel s, iterator const& it) noexcept { return (it != s); }

            // ++it
            iterator& operator++()
            {
                m_coroutine.resume();
                if (m_coroutine.done())
                    m_coroutine.promise().rethrow_if_exception();
                return *this;
            }

            // it++
            iterator& operator++(int) { return operator++(); }

            reference_type operator*() const noexcept { return m_coroutine.promise().value(); }

            pointer_type operator->() const noexcept { return std::addressof(operator*()); }

        private:
            handle_type m_coroutine{ nullptr };
        };

        generator() = default;
        explicit generator(handle_type handle) : m_coroutine(handle) { }
        generator(generator const&) = delete;
        generator(generator&& other) noexcept : m_coroutine(std::exchange(other.m_coroutine, nullptr)) { }

        generator& operator=(generator const&) = delete;
        generator& operator=(generator&& other) noexcept
        {
            if (std::addressof(other) != this)
            {
                if (m_coroutine != nullptr)
                    m_coroutine.destroy();
                m_coroutine = std::exchange(other.m_coroutine, nullptr);
            }
            return *this;
        }

        ~generator()
        {
            if (m_coroutine != nullptr)
                m_coroutine.destroy();
        }

        iterator begin()
        {
            if (m_coroutine != nullptr)
            {
                m_coroutine.resume();
                if (m_coroutine.done())
                    m_coroutine.promise().rethrow_if_exception();
            }
            
            return iterator{ m_coroutine };
        }

        sentinel end() noexcept { return { }; }

    private:
        handle_type m_coroutine { nullptr };
    };
}