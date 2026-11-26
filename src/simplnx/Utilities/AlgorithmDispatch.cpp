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
  GInCoreExecutionCount.store(counts.InCore, std::memory_order_relaxed);
  GOutOfCoreExecutionCount.store(counts.OutOfCore, std::memory_order_relaxed);
  GInCoreOnInMemoryStoreExecutionCount.store(counts.InCoreOnInMemoryStore, std::memory_order_relaxed);
  GInCoreOnOutOfCoreStoreExecutionCount.store(counts.InCoreOnOutOfCoreStore, std::memory_order_relaxed);
  GOutOfCoreOnInMemoryStoreExecutionCount.store(counts.OutOfCoreOnInMemoryStore, std::memory_order_relaxed);
  GOutOfCoreOnOutOfCoreStoreExecutionCount.store(counts.OutOfCoreOnOutOfCoreStore, std::memory_order_relaxed);
}

AlgorithmPathExecutionCounts GetAlgorithmPathExecutionCounts()
{
  return {
      .InCore = GInCoreExecutionCount.load(std::memory_order_relaxed),
      .OutOfCore = GOutOfCoreExecutionCount.load(std::memory_order_relaxed),
      .InCoreOnInMemoryStore = GInCoreOnInMemoryStoreExecutionCount.load(std::memory_order_relaxed),
      .InCoreOnOutOfCoreStore = GInCoreOnOutOfCoreStoreExecutionCount.load(std::memory_order_relaxed),
      .OutOfCoreOnInMemoryStore = GOutOfCoreOnInMemoryStoreExecutionCount.load(std::memory_order_relaxed),
      .OutOfCoreOnOutOfCoreStore = GOutOfCoreOnOutOfCoreStoreExecutionCount.load(std::memory_order_relaxed),
  };
}
} // namespace nx::core
