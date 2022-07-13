#include <benchmark/benchmark.h>
#include <boost/context/detail/fcontext.hpp>

int value_stackful = 0;
void stackful(boost::context::detail::transfer_t t)
{
    boost::context::detail::transfer_t tt{ t.fctx, t.data };
    while(true)
    {
        benchmark::DoNotOptimize(++value_stackful);
        tt = boost::context::detail::jump_fcontext(tt.fctx, tt.data);
    }
}

void BM_Stackful(benchmark::State& state)
{
    char stack[4096];
    // stack grows downside, so the passing address is stack[4095], not stack[0]
    boost::context::detail::fcontext_t ctx = boost::context::detail::make_fcontext(&stack[4095], 4096, stackful);

    boost::context::detail::transfer_t t{ ctx, nullptr };
    for (auto _ : state)
        t = boost::context::detail::jump_fcontext(t.fctx, t.data);
}

BENCHMARK(BM_Stackful)->Unit(benchmark::TimeUnit::kNanosecond);

#include "coro/generator.h"

coro::generator<int> stackless()
{
    int value_stackless = 0;
    while(true)
    {
        co_yield value_stackless++;
    }
}

void BM_Stackless(benchmark::State& state)
{
    auto g = stackless();
    auto it = g.begin();
    for (auto _ : state)
        benchmark::DoNotOptimize(++it);
}

BENCHMARK(BM_Stackless)->Unit(benchmark::TimeUnit::kNanosecond);

BENCHMARK_MAIN();

/*
Run on (8 X 3600 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
-------------------------------------------------------
Benchmark             Time             CPU   Iterations
-------------------------------------------------------
BM_Stackful        31.4 ns         31.5 ns     21333333
BM_Stackless       3.55 ns         3.53 ns    203636364
*/
