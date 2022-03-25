#include "coro/generator.h"
#include <string>
#include <fmt/core.h>

using namespace coro;

int main()
{
    auto g = []() -> generator<std::string> {  co_yield "Hello"; };
    for (auto const& e : g())
        fmt::print("{}\n", e);

    auto g1 = []() -> generator<int> { int i = 0; while(true) { co_yield i++; } };
    for (auto e : g1())
        if (e > 5) break;
        else fmt::print("{}\n", e);

    return 0;
}