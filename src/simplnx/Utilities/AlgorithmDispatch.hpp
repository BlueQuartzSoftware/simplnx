#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"

#include <initializer_list>
#include <utility>
#include <vector>

namespace nx::core
{

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @brief Checks the data-store type.
 * @param array Array to inspect.
 * @return True when the data store is out-of-core.
 */
inline bool IsOutOfCore(const IDataArray& array)
{
  return array.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore;
}

/**
 * @brief Checks the storage type of an array.
 *
 * IDataArray instances use IDataStore residency. INeighborList instances use
 * IListStore residency. Other IArray types do not expose residency and are
 * treated as in-core for dispatch.
 * @param array Array to inspect.
 * @return True when the array is out-of-core.
 */
inline bool IsOutOfCore(const IArray& array)
{
  if(const auto* dataArray = dynamic_cast<const IDataArray*>(&array); dataArray != nullptr)
  {
    return IsOutOfCore(*dataArray);
  }
  if(const auto* neighborList = dynamic_cast<const INeighborList*>(&array); neighborList != nullptr)
  {
    const auto* listStore = neighborList->getIListStore();
    return listStore != nullptr && listStore->isOutOfCore();
  }
  return false;
}

/**
 * @class AlgorithmArrayTargets
 * @brief Stores mixed array targets for dispatch selection.
 *
 * The wrapper owns the pointer list. It does not own the target arrays.
 * It allows mixed braced IArray and INeighborList targets. IDataArray-only and
 * empty braced lists retain their existing overload resolution.
 */
class AlgorithmArrayTargets
{
public:
  /**
   * @brief Copies non-owning array targets.
   * @param arrays Array targets to copy.
   */
  AlgorithmArrayTargets(std::initializer_list<const IArray*> arrays)
  : m_Arrays(arrays)
  {
  }

  /**
   * @brief Stores non-owning array targets.
   * @param arrays Array targets to move.
   */
  explicit AlgorithmArrayTargets(std::vector<const IArray*> arrays)
  : m_Arrays(std::move(arrays))
  {
  }

  const std::vector<const IArray*>& arrays() const noexcept
  {
    return m_Arrays;
  }

private:
  std::vector<const IArray*> m_Arrays;
};

/**
 * @brief Checks the storage types of data arrays.
 * @param arrays Array pointers to inspect. Null pointers are skipped.
 * @return True when an array is out-of-core.
 */
inline bool AnyOutOfCore(std::initializer_list<const IDataArray*> arrays)
{
  for(const auto* array : arrays)
  {
    if(array != nullptr && IsOutOfCore(*array))
    {
      return true;
    }
  }
  return false;
}

/**
 * @brief Checks the storage types of mixed array targets.
 * @param targets Array pointers to inspect. Null pointers are skipped.
 * @return True when a target is out-of-core.
 */
inline bool AnyOutOfCore(const AlgorithmArrayTargets& targets)
{
  for(const auto* array : targets.arrays())
  {
    if(array != nullptr && IsOutOfCore(*array))
    {
      return true;
    }
  }
  return false;
}

/**
 * @brief Returns the force-out-of-core test flag.
 *
 * The flag has process-wide mutable state. Set the flag before parallel work starts.
 * ForceOocAlgorithm() and ForceInCoreAlgorithm() select algorithm paths but do
 * not prove a storage and algorithm-path combination. Filter tests use
 * UnitTest::AlgorithmTestScope. The scope controls storage and checks execution
 * witnesses.
 * @warning The flag is not thread-safe.
 * @return Reference to the force-out-of-core test flag.
 */
SIMPLNX_EXPORT bool& ForceOocAlgorithm();

/**
 * @def SIMPLNX_TEST_ALGORITHM_PATH
 * @brief Selects filter-test algorithm scenarios.
 *
 * Value 0 selects both requested scenarios. Value 1 selects out-of-core
 * scenarios. Value 2 selects in-core scenarios.
 */
#ifndef SIMPLNX_TEST_ALGORITHM_PATH
#define SIMPLNX_TEST_ALGORITHM_PATH 0
#endif

/**
 * @class ForceOocAlgorithmGuard
 * @brief Restores the force-out-of-core test flag at scope exit.
 *
 * The guard changes the flag during its lifetime. It restores the prior value
 * when it is destroyed.
 * @warning The guard is not thread-safe. Use it before parallel work starts.
 */
class ForceOocAlgorithmGuard
{
public:
  /**
   * @brief Sets the force-out-of-core test flag.
   * @param force Test flag value to set.
   */
  ForceOocAlgorithmGuard(bool force)
  : m_Original(ForceOocAlgorithm())
  {
    ForceOocAlgorithm() = force;
  }

  /**
   * @brief Restores the prior force-out-of-core test flag.
   */
  ~ForceOocAlgorithmGuard()
  {
    ForceOocAlgorithm() = m_Original;
  }

  ForceOocAlgorithmGuard(const ForceOocAlgorithmGuard&) = delete;
  ForceOocAlgorithmGuard(ForceOocAlgorithmGuard&&) = delete;
  ForceOocAlgorithmGuard& operator=(const ForceOocAlgorithmGuard&) = delete;
  ForceOocAlgorithmGuard& operator=(ForceOocAlgorithmGuard&&) = delete;

private:
  bool m_Original = false;
};

/**
 * @brief Returns the force-in-core test flag.
 *
 * The in-core flag overrides storage detection and the out-of-core flag.
 * @warning The flag is not thread-safe.
 * @return Reference to the force-in-core test flag.
 */
SIMPLNX_EXPORT bool& ForceInCoreAlgorithm();

/**
 * @class ForceInCoreAlgorithmGuard
 * @brief Restores the force-in-core test flag at scope exit.
 *
 * The guard forces the in-core algorithm during its lifetime.
 * @warning The guard is not thread-safe. Use it before parallel work starts.
 */
class ForceInCoreAlgorithmGuard
{
public:
  /**
   * @brief Forces the in-core test path.
   */
  ForceInCoreAlgorithmGuard()
  : m_Original(ForceInCoreAlgorithm())
  {
    ForceInCoreAlgorithm() = true;
  }

  /**
   * @brief Restores the prior force-in-core test flag.
   */
  ~ForceInCoreAlgorithmGuard()
  {
    ForceInCoreAlgorithm() = m_Original;
  }

  ForceInCoreAlgorithmGuard(const ForceInCoreAlgorithmGuard&) = delete;
  ForceInCoreAlgorithmGuard(ForceInCoreAlgorithmGuard&&) = delete;
  ForceInCoreAlgorithmGuard& operator=(const ForceInCoreAlgorithmGuard&) = delete;
  ForceInCoreAlgorithmGuard& operator=(ForceInCoreAlgorithmGuard&&) = delete;

private:
  bool m_Original = false;
};

/**
 * @enum AlgorithmPath
 * @brief The enum identifies an implementation selected by a dispatched algorithm.
 */
enum class AlgorithmPath : uint8
{
  InCore,   ///< Selects the in-core algorithm.
  OutOfCore ///< Selects the out-of-core algorithm.
};

/**
 * @struct AlgorithmPathExecutionCounts
 * @brief The struct stores the number of times each dispatched implementation runs.
 */
struct AlgorithmPathExecutionCounts
{
  uint64 InCore = 0;
  uint64 OutOfCore = 0;
  uint64 InCoreOnInMemoryStore = 0;
  uint64 InCoreOnOutOfCoreStore = 0;
  uint64 OutOfCoreOnInMemoryStore = 0;
  uint64 OutOfCoreOnOutOfCoreStore = 0;
};

/**
 * @brief Records a dispatch path and storage type.
 * @param path Selected algorithm path.
 * @param usesOutOfCoreStore True when a dispatch target is out-of-core.
 *
 * Relaxed atomic counters make concurrent recording data-race-free. The counters
 * are test witnesses and do not synchronize algorithm work.
 */
SIMPLNX_EXPORT void RecordAlgorithmPathExecution(AlgorithmPath path, bool usesOutOfCoreStore);

/**
 * @brief Resets dispatch execution counts.
 *
 * Call this function only when dispatch recording is idle if tests require one
 * coherent multi-counter state.
 */
SIMPLNX_EXPORT void ResetAlgorithmPathExecutionCounts();

/**
 * @brief Replaces dispatch execution counts.
 * @param counts Replacement count values.
 *
 * Unit tests use this function to restore state after execution. Production
 * algorithms call RecordAlgorithmPathExecution() instead.
 * Individual stores are atomic, but replacement of all counters is not one
 * atomic transaction. Call this function when dispatch recording is idle.
 */
SIMPLNX_EXPORT void SetAlgorithmPathExecutionCounts(const AlgorithmPathExecutionCounts& counts);

/**
 * @brief Returns dispatch execution counts.
 * @return Aggregate and algorithm-store combination counts.
 *
 * Individual loads are atomic. Concurrent recording can produce a snapshot whose
 * fields represent different instants.
 */
SIMPLNX_EXPORT AlgorithmPathExecutionCounts GetAlgorithmPathExecutionCounts();

/**
 * @brief Dispatches between in-core and out-of-core algorithms.
 *
 * The selected algorithm receives forwarded constructor arguments. The function
 * records the selected path for the unit-test execution witness.
 *
 * In-core algorithms can use random access. Random access on disk-backed chunks
 * can repeat load and eviction cycles. Out-of-core algorithms use
 * chunk-sequential or local access.
 *
 * ForceInCoreAlgorithm() has precedence over storage detection and
 * ForceOocAlgorithm(). The function otherwise selects OocAlgo when an array is
 * out-of-core or ForceOocAlgorithm() is true.
 *
 * @tparam InCoreAlgo In-core algorithm class.
 * @tparam OocAlgo Out-of-core algorithm class.
 * @tparam ArgsT Forwarded constructor argument types.
 * @param arrays Arrays for storage detection.
 * @param args Arguments for the selected constructor.
 * @return Result from the selected algorithm.
 */
template <typename InCoreAlgo, typename OocAlgo, typename... ArgsT>
Result<> DispatchAlgorithm(std::initializer_list<const IDataArray*> arrays, ArgsT&&... args)
{
  const bool usesOutOfCoreStore = AnyOutOfCore(arrays);
  const bool useOutOfCoreAlgorithm = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCoreAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

  if(useOutOfCoreAlgorithm)
  {
    return OocAlgo(std::forward<ArgsT>(args)...)();
  }
  return InCoreAlgo(std::forward<ArgsT>(args)...)();
}

/**
 * @brief Dispatches between algorithms for mixed array targets.
 *
 * The in-core flag has precedence. An out-of-core target or flag selects the
 * out-of-core algorithm. The function records the selected path for tests.
 * @tparam InCoreAlgo In-core algorithm class.
 * @tparam OocAlgo Out-of-core algorithm class.
 * @tparam ArgsT Forwarded constructor argument types.
 * @param targets Arrays for storage detection.
 * @param args Arguments for the selected constructor.
 * @return Result from the selected algorithm.
 */
template <typename InCoreAlgo, typename OocAlgo, typename... ArgsT>
Result<> DispatchAlgorithm(const AlgorithmArrayTargets& targets, ArgsT&&... args)
{
  const bool usesOutOfCoreStore = AnyOutOfCore(targets);
  const bool useOutOfCoreAlgorithm = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCoreAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

  if(useOutOfCoreAlgorithm)
  {
    return OocAlgo(std::forward<ArgsT>(args)...)();
  }
  else
  {
    return InCoreAlgo(std::forward<ArgsT>(args)...)();
  }
}

} // namespace nx::core
