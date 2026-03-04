#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/simplnx_export.hpp"

#include <vector>

namespace nx::core
{
class SIMPLNX_EXPORT ParallelAlgorithm
{
public:
  using AlgorithmArrays = std::vector<const AbstractDataArray*>;
  using AlgorithmStores = std::vector<const IDataStore*>;

  ParallelAlgorithm(const ParallelAlgorithm&) = default;
  ParallelAlgorithm(ParallelAlgorithm&&) noexcept = default;
  ParallelAlgorithm& operator=(const ParallelAlgorithm&) = default;
  ParallelAlgorithm& operator=(ParallelAlgorithm&&) noexcept = default;

  /**
   * @brief Returns true if parallelization is enabled.  Returns false otherwise.
   * @return
   */
  [[nodiscard]] bool getParallelizationEnabled() const;

  /**
   * @brief Sets whether parallelization is enabled.
   * @param doParallel
   */
  void setParallelizationEnabled(bool doParallel);

  void requireArraysInMemory(const AlgorithmArrays& arrays);

  void requireStoresInMemory(const AlgorithmStores& arrays);

protected:
  ParallelAlgorithm();
  ~ParallelAlgorithm();

private:
#ifdef SIMPLNX_ENABLE_MULTICORE
  bool m_RunParallel = true;
#else
  bool m_RunParallel = false;
#endif
};
} // namespace nx::core
