#pragma once

#include "complex/Common/Range3D.hpp"
#include "complex/complex_export.hpp"

#ifdef COMPLEX_ENABLE_MULTICORE
#include <tbb/blocked_range3d.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>
#endif

#include <array>
#include <cstddef>

namespace complex
{
/**
 * @brief The ParallelDataAlgorithm class handles parallelization across data-based algorithms.
 * A range is required, as well as an object with a matching function operator.  This class
 * utilizes TBB for parallelization and will fallback to non-parallelization if it is not
 * available or the parallelization is disabled.
 */
class COMPLEX_EXPORT ParallelData3DAlgorithm
{
public:
  using RangeType = Range3D;

  ParallelData3DAlgorithm();
  ~ParallelData3DAlgorithm();

  ParallelData3DAlgorithm(const ParallelData3DAlgorithm&) = delete;
  ParallelData3DAlgorithm(ParallelData3DAlgorithm&&) noexcept = delete;
  ParallelData3DAlgorithm& operator=(const ParallelData3DAlgorithm&) = delete;
  ParallelData3DAlgorithm& operator=(ParallelData3DAlgorithm&&) noexcept = delete;

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

  /**
   * @brief Returns the range to operate over.
   * @return
   */
  RangeType getRange() const;

  /**
   * @brief Sets the range to operate over.
   * @param range3D
   */
  void setRange(const RangeType& range);

  /**
   * @brief Sets the range to operate over.
   * @param range3D
   */
  void setRange(size_t xMax, size_t yMax, size_t zMax);

  /**
   * @brief Runs the data algorithm.  Parallelization is used if appropriate.
   * @param body
   */
  template <typename Body>
  void execute(const Body& body)
  {
#ifdef COMPLEX_ENABLE_MULTICORE
    if(m_RunParallel)
    {
      tbb::auto_partitioner partitioner;
      tbb::blocked_range3d<size_t, size_t, size_t> tbbRange(m_Range[4], m_Range[5], m_Range[2], m_Range[3], m_Range[0], m_Range[1]);
      tbb::parallel_for(tbbRange, body, partitioner);
    }
    else
#endif
    // Run non-parallel operation
    {
      body(m_Range);
    }
  }

private:
  RangeType m_Range;
#ifdef COMPLEX_ENABLE_MULTICORE
  bool m_RunParallel = true;
#else
  bool m_RunParallel = false;
#endif
};
} // namespace complex
