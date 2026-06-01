#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"

#include <array>
#include <initializer_list>

namespace nx::core
{

/**
 * @brief Checks whether an IDataArray is backed by out-of-core (chunked) storage.
 *
 * Returns true when the array's data store reports StoreType::OutOfCore,
 * indicating that data lives on disk in compressed chunks rather than in a
 * contiguous in-memory buffer.
 *
 * @param array The data array to check
 * @return true if the array uses chunked/OOC storage
 */
inline bool IsOutOfCore(const IDataArray& array)
{
  return array.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore;
}

/**
 * @brief Checks whether any of the given IDataArrays are backed by out-of-core storage.
 *
 * Filters often operate on multiple input and output arrays. If any of them use
 * chunked storage, the OOC algorithm path should be used to avoid chunk thrashing.
 *
 * @param arrays List of pointers to data arrays to check (nullptrs are skipped)
 * @return true if any non-null array uses chunked/OOC storage
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
 * @brief Returns a reference to the global flag that forces DispatchAlgorithm
 *        to always select the out-of-core algorithm, regardless of storage type.
 *
 * This is primarily used in unit tests to exercise the OOC algorithm path
 * even when data is stored in-core. The flag is backed by a function-local
 * static, so it persists for the lifetime of the process.
 *
 * @warning This flag is NOT thread-safe. It should only be set from the main
 *          test thread before any parallel work begins. Use ForceOocAlgorithmGuard
 *          for RAII-safe toggling in tests.
 *
 * @return Reference to the static force flag
 */
inline bool& ForceOocAlgorithm()
{
  static bool s_force = false;
  return s_force;
}

/**
 * @brief Integer array of forceOoc values for Catch2 GENERATE(from_range(...)).
 *
 * Controlled by CMake option SIMPLNX_TEST_ALGORITHM_PATH (passed as a
 * compile definition to test targets):
 *   0 (Both)       - {0, 1}: tests both in-core and OOC paths (default)
 *   1 (OocOnly)    - {1}:    tests only OOC path (use for OOC builds)
 *   2 (InCoreOnly) - {0}:    tests only in-core path (quick validation)
 *
 * Uses int instead of bool because Catch2 v2's FixedValuesGenerator
 * does not support bool due to std::vector<bool> specialization.
 *
 * Usage in tests:
 * @code
 *   bool forceOoc = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
 *   const nx::core::ForceOocAlgorithmGuard guard(forceOoc);
 * @endcode
 *
 * Set via: cmake -DSIMPLNX_TEST_ALGORITHM_PATH=1 ...
 */
#ifndef SIMPLNX_TEST_ALGORITHM_PATH
#define SIMPLNX_TEST_ALGORITHM_PATH 0
#endif

// clang-format off
#if SIMPLNX_TEST_ALGORITHM_PATH == 1
inline const std::array<int, 1> k_ForceOocTestValues = {1};
#elif SIMPLNX_TEST_ALGORITHM_PATH == 2
inline const std::array<int, 1> k_ForceOocTestValues = {0};
#else
inline const std::array<int, 2> k_ForceOocTestValues = {0, 1};
#endif
// clang-format on

/**
 * @brief RAII guard that sets ForceOocAlgorithm() on construction and
 *        restores the previous value on destruction.
 *
 * The guard captures the current value of ForceOocAlgorithm() when constructed,
 * overrides it with the requested value, and restores the original value when
 * the guard goes out of scope. This ensures the global flag is always cleaned
 * up, even if the test throws an exception or fails early.
 *
 * Copy and move operations are deleted to prevent accidental double-restore
 * of the original value, which would corrupt the global flag state.
 *
 * @warning Not thread-safe. The underlying flag is a bare static bool with
 *          no synchronization. In Catch2 tests this is safe because each
 *          TEST_CASE runs on the main thread, but do not use this guard
 *          from worker threads.
 *
 * Usage in tests:
 * @code
 *   bool forceOoc = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
 *   const nx::core::ForceOocAlgorithmGuard guard(forceOoc);
 * @endcode
 */
class ForceOocAlgorithmGuard
{
public:
  ForceOocAlgorithmGuard(bool force)
  : m_Original(ForceOocAlgorithm())
  {
    // Override the global flag for the duration of this guard's lifetime
    ForceOocAlgorithm() = force;
  }

  ~ForceOocAlgorithmGuard()
  {
    // Restore the original value so subsequent tests start with a clean state
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
 * @brief Returns a reference to the global flag that forces DispatchAlgorithm
 *        to always select the in-core algorithm, overriding storage-type detection.
 *
 * This is primarily used in unit tests to exercise the in-core algorithm path
 * even when data is stored out-of-core (e.g., loaded from HDF5 in an OOC build).
 * The flag is backed by a function-local static, so it persists for the lifetime
 * of the process.
 *
 * ForceInCoreAlgorithm() takes the highest precedence in DispatchAlgorithm:
 * when set to true, neither AnyOutOfCore() nor ForceOocAlgorithm() can
 * override it. This allows tests to verify in-core correctness even when
 * running in an OOC-enabled build where arrays may be loaded as chunked stores.
 *
 * @warning Not thread-safe. See ForceOocAlgorithm() for details.
 *
 * @return Reference to the static force flag
 */
inline bool& ForceInCoreAlgorithm()
{
  static bool s_force = false;
  return s_force;
}

/**
 * @brief RAII guard that unconditionally sets ForceInCoreAlgorithm() to true
 *        on construction and restores the previous value on destruction.
 *
 * Unlike ForceOocAlgorithmGuard, this guard always forces in-core mode and
 * does not accept a boolean parameter. This is intentional: forcing in-core
 * is an override that should only be applied deliberately in tests that need
 * to verify in-core behavior in an OOC-enabled build.
 *
 * Copy and move operations are deleted to prevent accidental double-restore.
 *
 * @warning Not thread-safe. See ForceOocAlgorithmGuard for details.
 *
 * Usage in tests:
 * @code
 *   const nx::core::ForceInCoreAlgorithmGuard guard;
 * @endcode
 */
class ForceInCoreAlgorithmGuard
{
public:
  ForceInCoreAlgorithmGuard()
  : m_Original(ForceInCoreAlgorithm())
  {
    // Unconditionally force in-core dispatch for the guard's lifetime
    ForceInCoreAlgorithm() = true;
  }

  ~ForceInCoreAlgorithmGuard()
  {
    // Restore the original value so subsequent tests start with a clean state
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
 * @brief Dispatches between two algorithm classes based on whether any of the
 *        given data arrays use out-of-core (chunked) storage, or if the global
 *        ForceOocAlgorithm() flag is set.
 *
 * Some algorithms that perform well on in-memory data (e.g. BFS flood fill with
 * random access) become extremely slow when data is stored in disk-backed chunks,
 * because each random access may trigger a chunk load/evict cycle. In these cases,
 * a different algorithm (e.g. scanline CCL with sequential chunk access) can be
 * orders of magnitude faster for OOC data.
 *
 * Selection logic (evaluated in order):
 *   1. ForceInCoreAlgorithm() == true  -> always use InCoreAlgo
 *   2. AnyOutOfCore(arrays) == true    -> use OocAlgo
 *   3. ForceOocAlgorithm() == true     -> use OocAlgo
 *   4. Otherwise                       -> use InCoreAlgo
 *
 * @tparam InCoreAlgo Algorithm class optimized for in-memory data
 * @tparam OocAlgo Algorithm class optimized for out-of-core (chunked) data
 * @tparam ArgsT Constructor argument types (must be identical for both algorithms)
 * @param arrays The data arrays used to detect storage type (OOC if any is OOC)
 * @param args Constructor arguments forwarded to the selected algorithm
 * @return Result<> from the selected algorithm's operator()()
 */
template <typename InCoreAlgo, typename OocAlgo, typename... ArgsT>
Result<> DispatchAlgorithm(std::initializer_list<const IDataArray*> arrays, ArgsT&&... args)
{
  // Selection priority (highest to lowest):
  //   1. ForceInCoreAlgorithm == true  -> InCoreAlgo  (test override, wins over everything)
  //   2. AnyOutOfCore(arrays) == true  -> OocAlgo     (real OOC data detected at runtime)
  //   3. ForceOocAlgorithm == true     -> OocAlgo     (test override for exercising OOC path)
  //   4. Default                       -> InCoreAlgo  (all data is in-memory)
  if(!ForceInCoreAlgorithm() && (AnyOutOfCore(arrays) || ForceOocAlgorithm()))
  {
    // Construct the OOC algorithm with the forwarded args and invoke operator()()
    return OocAlgo(std::forward<ArgsT>(args)...)();
  }
  else
  {
    // Construct the in-core algorithm with the forwarded args and invoke operator()()
    return InCoreAlgo(std::forward<ArgsT>(args)...)();
  }
}

} // namespace nx::core
