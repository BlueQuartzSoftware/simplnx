#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"

#include <initializer_list>

namespace nx::core
{

/**
 * @brief Checks whether an IDataArray is backed by out-of-core (chunked) storage.
 *
 * Returns true when the array's data store reports a chunk shape (e.g. ZarrStore),
 * indicating that data lives on disk in compressed chunks rather than in a
 * contiguous in-memory buffer.
 *
 * @param array The data array to check
 * @return true if the array uses chunked/OOC storage
 */
inline bool IsOutOfCore(const IDataArray& array)
{
  return array.getIDataStoreRef().getChunkShape().has_value();
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
 * even when data is stored in-core. Use ForceOocAlgorithmGuard for RAII-safe
 * toggling in tests.
 *
 * @return Reference to the static force flag
 */
inline bool& ForceOocAlgorithm()
{
  static bool s_force = false;
  return s_force;
}

/**
 * @brief RAII guard that sets ForceOocAlgorithm() on construction and
 *        restores the previous value on destruction.
 *
 * Usage in tests with Catch2 GENERATE:
 * @code
 *   bool forceOoc = GENERATE(false, true);
 *   const nx::core::ForceOocAlgorithmGuard guard(forceOoc);
 *   // ... test body runs with both algorithm paths ...
 * @endcode
 */
class ForceOocAlgorithmGuard
{
public:
  ForceOocAlgorithmGuard(bool force)
  : m_Original(ForceOocAlgorithm())
  {
    ForceOocAlgorithm() = force;
  }

  ~ForceOocAlgorithmGuard()
  {
    ForceOocAlgorithm() = m_Original;
  }

  ForceOocAlgorithmGuard(const ForceOocAlgorithmGuard&) = delete;
  ForceOocAlgorithmGuard(ForceOocAlgorithmGuard&&) = delete;
  ForceOocAlgorithmGuard& operator=(const ForceOocAlgorithmGuard&) = delete;
  ForceOocAlgorithmGuard& operator=(ForceOocAlgorithmGuard&&) = delete;

private:
  bool m_Original;
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
 * This function checks the storage type of the given arrays and the global force
 * flag. If *any* array is out-of-core or ForceOocAlgorithm() is true, the OOC
 * algorithm is selected. Callers should pass all input and output arrays the
 * filter operates on. Both algorithm classes must:
 *   - Be constructible from the same ArgsT... parameter pack
 *   - Provide operator()() returning Result<>
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
  if(AnyOutOfCore(arrays) || ForceOocAlgorithm())
  {
    return OocAlgo(std::forward<ArgsT>(args)...)();
  }
  else
  {
    return InCoreAlgo(std::forward<ArgsT>(args)...)();
  }
}

} // namespace nx::core
