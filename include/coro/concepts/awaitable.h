#include <coroutine>
#include <type_traits>
#include <concepts>

namespace coro
{
    namespace concepts
    {
        namespace detail
        {
            template <typename T>
            concept has_member_co_await = requires(T &&t)
            {
                static_cast<T &&>(t).operator co_await(); // == std::forward<T>(t).operator co_await()
            };

            template <typename T>
            concept has_non_member_co_await = requires(T &&t)
            {
                operator co_await(std::forward<T>(t)); // == operator co_await(static_cast<T&&>(t))
            };

            template <typename A>
            struct GetAwaiter : std::type_identity<A> { };

            template <typename A>
            requires has_member_co_await<A>
            struct GetAwaiter<A> : std::type_identity<decltype(std::declval<A>().operator co_await())> { };

            template <typename A>
            requires(!has_member_co_await<A>) && has_non_member_co_await<A> struct GetAwaiter<A> : std::type_identity<decltype(operator co_await(std::declval<A>()))> { };

            template <typename A>
            using Awaiter_t = typename GetAwaiter<A>::type;

            template <typename T>
            struct is_valid_await_suspend_return_type : std::false_type { };

            template <>
            struct is_valid_await_suspend_return_type<void> : std::true_type { };

            template <>
            struct is_valid_await_suspend_return_type<bool> : std::true_type { };

            template <typename Promise>
            struct is_valid_await_suspend_return_type<std::coroutine_handle<Promise>> : std::true_type { };

            template <typename A>
            concept AwaitSuspendReturnType = is_valid_await_suspend_return_type<A>::value;
        }

        template <typename A>
        concept Awaitable = requires(detail::Awaiter_t<A> a, std::coroutine_handle<void> h) // convert A to Awaiter_t
        {
            { a.await_ready() } -> std::convertible_to<bool>;
            a.await_suspend(h);
            requires detail::AwaitSuspendReturnType<decltype(a.await_suspend(h))>;
            a.await_resume();
        };

        template<Awaitable A>
        using AwaitResult = decltype(std::declval<detail::Awaiter_t<A>>().await_resume());
    }
}