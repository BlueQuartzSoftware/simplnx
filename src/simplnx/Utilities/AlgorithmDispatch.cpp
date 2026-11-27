#include "AlgorithmDispatch.hpp"

#include <atomic>

namespace
{
std::atomic<nx::core::uint64> GInCoreExecutionCount = 0;
std::atomic<nx::core::uint64> GOutOfCoreExecutionCount = 0;
std::atomic<nx::core::uint64> GInCoreOnInMemoryStoreExecutionCount = 0;
std::atomic<nx::core::uint64> GInCoreOnOutOfCoreStoreExecutionCount = 0;
std::atomic<nx::core::uint64> GOutOfCoreOnInMemoryStoreExecutionCount = 0;
std::atomic<nx::core::uint64> GOutOfCoreOnOutOfCoreStoreExecutionCount = 0;
} // namespace

namespace nx::core
{
bool& ForceOocAlgorithm()
{
  static bool sForceOocAlgorithm = false;
  return sForceOocAlgorithm;
}

bool& ForceInCoreAlgorithm()
{
  static bool sForceInCoreAlgorithm = false;
  return sForceInCoreAlgorithm;
}

void RecordAlgorithmPathExecution(AlgorithmPath path, bool usesOutOfCoreStore)
{
  if(path == AlgorithmPath::OutOfCore)
  {
    GOutOfCoreExecutionCount.fetch_add(1, std::memory_order_relaxed);
    auto& scenarioCount = usesOutOfCoreStore ? GOutOfCoreOnOutOfCoreStoreExecutionCount : GOutOfCoreOnInMemoryStoreExecutionCount;
    scenarioCount.fetch_add(1, std::memory_order_relaxed);
    return;
  }

  GInCoreExecutionCount.fetch_add(1, std::memory_order_relaxed);
  auto& scenarioCount = usesOutOfCoreStore ? GInCoreOnOutOfCoreStoreExecutionCount : GInCoreOnInMemoryStoreExecutionCount;
  scenarioCount.fetch_add(1, std::memory_order_relaxed);
}

void ResetAlgorithmPathExecutionCounts()
{
  SetAlgorithmPathExecutionCounts({});
}

void SetAlgorithmPathExecutionCounts(const AlgorithmPathExecutionCounts& counts)
{
  GInCoreExecutionCount.store(counts.in_core, std::memory_order_relaxed);
  GOutOfCoreExecutionCount.store(counts.out_of_core, std::memory_order_relaxed);
  GInCoreOnInMemoryStoreExecutionCount.store(counts.in_core_on_in_memory_store, std::memory_order_relaxed);
  GInCoreOnOutOfCoreStoreExecutionCount.store(counts.in_core_on_out_of_core_store, std::memory_order_relaxed);
  GOutOfCoreOnInMemoryStoreExecutionCount.store(counts.out_of_core_on_in_memory_store, std::memory_order_relaxed);
  GOutOfCoreOnOutOfCoreStoreExecutionCount.store(counts.out_of_core_on_out_of_core_store, std::memory_order_relaxed);
}

AlgorithmPathExecutionCounts GetAlgorithmPathExecutionCounts()
{
  return {
      .in_core = GInCoreExecutionCount.load(std::memory_order_relaxed),
      .out_of_core = GOutOfCoreExecutionCount.load(std::memory_order_relaxed),
      .in_core_on_in_memory_store = GInCoreOnInMemoryStoreExecutionCount.load(std::memory_order_relaxed),
      .in_core_on_out_of_core_store = GInCoreOnOutOfCoreStoreExecutionCount.load(std::memory_order_relaxed),
      .out_of_core_on_in_memory_store = GOutOfCoreOnInMemoryStoreExecutionCount.load(std::memory_order_relaxed),
      .out_of_core_on_out_of_core_store = GOutOfCoreOnOutOfCoreStoreExecutionCount.load(std::memory_order_relaxed),
  };
}
} // namespace nx::core
