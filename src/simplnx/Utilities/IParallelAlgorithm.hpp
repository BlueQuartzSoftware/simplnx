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
   * @brief Sets whether parallelization is enabled.
   * @param doParallel
   */
  void setParallelizationEnabled(bool doParallel);

  void requireArraysInMemory(const AlgorithmArrays& arrays);

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

namespace detail
{
bool CheckArraysInMemory(const nx::core::IParallelAlgorithm::AlgorithmArrays& arrays);
}
} // namespace nx::core
