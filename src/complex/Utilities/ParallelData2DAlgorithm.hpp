#pragma once

#include <array>

#include "complex/Common/Range2D.hpp"
#include "complex/complex_export.hpp"

#ifdef COMPLEX_ENABLE_MULTICORE
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>
#endif

#include <array>
#include <cstddef>

namespace complex
{

/**
 * @brief The ParallelData2DAlgorithm class handles parallelization across 2D data-based algorithms.
 * A range is required, as well as an object with a matching function operator.  This class
 * utilizes TBB for parallelization and will fallback to non-parallelization if it is not
 * available or the parallelization is disabled.
 */
class COMPLEX_EXPORT ParallelData2DAlgorithm
{
public:
  using RangeType = Range2D;

  ParallelData2DAlgorithm();
  ~ParallelData2DAlgorithm();

  ParallelData2DAlgorithm(const ParallelData2DAlgorithm&) = default;
  ParallelData2DAlgorithm(ParallelData2DAlgorithm&&) noexcept = default;
  ParallelData2DAlgorithm& operator=(const ParallelData2DAlgorithm&) = default;
  ParallelData2DAlgorithm& operator=(ParallelData2DAlgorithm&&) noexcept = default;

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
   * @param range2D
   */
  void setRange(const RangeType& range);

  /**
   * @brief Sets the range to operate over
   * @param minCols
   * @param maxCols
   * @param minRows
   * @param maxRows
   */
  void setRange(size_t minCols, size_t maxCols, size_t minRows, size_t maxRows);

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
      tbb::blocked_range2d<size_t, size_t> tbbRange(m_Range.minRow(), m_Range.maxRow(), m_Range.minCol(), m_Range.maxCol());
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
