#include <benchmark/benchmark.h>

namespace
{
void BenchmarkCaseA(benchmark::State& state)
{
  for(auto _ : state)
  {
    std::string x("foo");
    benchmark::DoNotOptimize(x);
  }
}

void BenchmarkCaseB(benchmark::State& state)
{
  std::string x = "foo";
  for(auto _ : state)
  {
    std::string copy(x);
    benchmark::DoNotOptimize(copy);
  }
}
} // namespace

BENCHMARK(BenchmarkCaseA);
BENCHMARK(BenchmarkCaseB);

BENCHMARK_MAIN();
