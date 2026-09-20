#include "AlgorithmDispatch.hpp"

#include <atomic>

namespace
{
std::atomic<nx::core::uint64> g_InCoreExecutionCount = 0;
std::atomic<nx::core::uint64> g_OutOfCoreExecutionCount = 0;
std::atomic<nx::core::uint64> g_InCoreOnInMemoryStoreExecutionCount = 0;
std::atomic<nx::core::uint64> g_InCoreOnOutOfCoreStoreExecutionCount = 0;
std::atomic<nx::core::uint64> g_OutOfCoreOnInMemoryStoreExecutionCount = 0;
std::atomic<nx::core::uint64> g_OutOfCoreOnOutOfCoreStoreExecutionCount = 0;
} // namespace

namespace nx::core
{
bool& ForceOocAlgorithm()
{
  static bool s_ForceOocAlgorithm = false;
  return s_ForceOocAlgorithm;
}

bool& ForceInCoreAlgorithm()
{
  static bool s_ForceInCoreAlgorithm = false;
  return s_ForceInCoreAlgorithm;
}

void RecordAlgorithmPathExecution(AlgorithmPath path, bool usesOutOfCoreStore)
{
  if(path == AlgorithmPath::OutOfCore)
  {
    g_OutOfCoreExecutionCount.fetch_add(1, std::memory_order_relaxed);
    auto& scenarioCount = usesOutOfCoreStore ? g_OutOfCoreOnOutOfCoreStoreExecutionCount : g_OutOfCoreOnInMemoryStoreExecutionCount;
    scenarioCount.fetch_add(1, std::memory_order_relaxed);
    return;
  }

  g_InCoreExecutionCount.fetch_add(1, std::memory_order_relaxed);
  auto& scenarioCount = usesOutOfCoreStore ? g_InCoreOnOutOfCoreStoreExecutionCount : g_InCoreOnInMemoryStoreExecutionCount;
  scenarioCount.fetch_add(1, std::memory_order_relaxed);
}

void ResetAlgorithmPathExecutionCounts()
{
  SetAlgorithmPathExecutionCounts({});
}

void SetAlgorithmPathExecutionCounts(const AlgorithmPathExecutionCounts& counts)
{
  g_InCoreExecutionCount.store(counts.InCore, std::memory_order_relaxed);
  g_OutOfCoreExecutionCount.store(counts.OutOfCore, std::memory_order_relaxed);
  g_InCoreOnInMemoryStoreExecutionCount.store(counts.InCoreOnInMemoryStore, std::memory_order_relaxed);
  g_InCoreOnOutOfCoreStoreExecutionCount.store(counts.InCoreOnOutOfCoreStore, std::memory_order_relaxed);
  g_OutOfCoreOnInMemoryStoreExecutionCount.store(counts.OutOfCoreOnInMemoryStore, std::memory_order_relaxed);
  g_OutOfCoreOnOutOfCoreStoreExecutionCount.store(counts.OutOfCoreOnOutOfCoreStore, std::memory_order_relaxed);
}

AlgorithmPathExecutionCounts GetAlgorithmPathExecutionCounts()
{
  return {
      g_InCoreExecutionCount.load(std::memory_order_relaxed),
      g_OutOfCoreExecutionCount.load(std::memory_order_relaxed),
      g_InCoreOnInMemoryStoreExecutionCount.load(std::memory_order_relaxed),
      g_InCoreOnOutOfCoreStoreExecutionCount.load(std::memory_order_relaxed),
      g_OutOfCoreOnInMemoryStoreExecutionCount.load(std::memory_order_relaxed),
      g_OutOfCoreOnOutOfCoreStoreExecutionCount.load(std::memory_order_relaxed),
  };
}
} // namespace nx::core
