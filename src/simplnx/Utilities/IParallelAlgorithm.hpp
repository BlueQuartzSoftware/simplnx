#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/simplnx_export.hpp"

#include <vector>

namespace nx::core
{
class SIMPLNX_EXPORT IParallelAlgorithm
{
public:
  using AlgorithmArrays = std::vector<const IDataArray*>;
  using AlgorithmStores = std::vector<const IDataStore*>;

  IParallelAlgorithm(const IParallelAlgorithm&) = default;
  IParallelAlgorithm(IParallelAlgorithm&&) noexcept = default;
  IParallelAlgorithm& operator=(const IParallelAlgorithm&) = default;
  IParallelAlgorithm& operator=(IParallelAlgorithm&&) noexcept = default;

  /**
   * @brief Returns true if parallelization is enabled.  Returns false otherwise.
   * @return
   */
  [[nodiscard]] bool getParallelizationEnabled() const;

  /**
   * @brief Sets the parallelization state explicitly.
   * @param doParallel Requested parallelization state.
   *
   * When doParallel is true, this call resets restrictions from earlier
   * require calls in multicore builds.
   * Builds without SIMPLNX_ENABLE_MULTICORE keep parallelization disabled.
   */
  void setParallelizationEnabled(bool doParallel);

  /**
   * @brief Requires array stores to be in memory for parallel work.
   * @param arrays Arrays whose stores must be in memory.
   *
   * Each call preserves a disabled state. Call
   * setParallelizationEnabled(true) to reset the state in a multicore build.
   */
  void requireArraysInMemory(const AlgorithmArrays& arrays);

  /**
   * @brief Requires stores to be in memory for parallel work.
   * @param arrays Stores that must be in memory.
   *
   * Each call preserves a disabled state. Call
   * setParallelizationEnabled(true) to reset the state in a multicore build.
   */
  void requireStoresInMemory(const AlgorithmStores& arrays);

protected:
  IParallelAlgorithm();
  ~IParallelAlgorithm();

private:
#ifdef SIMPLNX_ENABLE_MULTICORE
  bool m_RunParallel = true;
#else
  bool m_RunParallel = false;
#endif
};
} // namespace nx::core
