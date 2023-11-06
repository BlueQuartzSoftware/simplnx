#pragma once

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/complex_export.hpp"

#include <vector>

namespace complex
{
class COMPLEX_EXPORT IParallelAlgorithm
{
public:
  using AlgorithmArrays = std::vector<const IDataArray*>;

  IParallelAlgorithm(const IParallelAlgorithm&) = default;
  IParallelAlgorithm(IParallelAlgorithm&&) noexcept = default;
  IParallelAlgorithm& operator=(const IParallelAlgorithm&) = default;
  IParallelAlgorithm& operator=(IParallelAlgorithm&&) noexcept = default;

  /**
   * @brief Returns true if parallelization is enabled.  Returns false otherwise.
   * @return
   */
  bool getParallelizationEnabled() const;

  /**
   * @brief Sets whether parallelization is enabled.
   * @param doParallel
   */
  void setParallelizationEnabled(bool doParallel);

  void requireArraysInMemory(const AlgorithmArrays& arrays);

protected:
  IParallelAlgorithm();
  ~IParallelAlgorithm();

private:
#ifdef COMPLEX_ENABLE_MULTICORE
  bool m_RunParallel = true;
#else
  bool m_RunParallel = false;
#endif
};
} // namespace complex
