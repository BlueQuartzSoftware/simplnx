#pragma once

#include <array>

#include "complex/Common/ComplexRange2D.hpp"
#include "complex/complex_export.hpp"

// complex MUST be included before this or the guard will block the include but not its uses below.
// This is consistent with previous behavior, only earlier parallelization split the includes between
// the corresponding .h and .cpp files.
#ifdef COMPLEX_ENABLE_MULTICORE
#include <tbb/blocked_range.h>
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
  using RangeType = ComplexRange2D;

  ParallelData2DAlgorithm();
  virtual ~ParallelData2DAlgorithm();

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

#ifdef COMPLEX_ENABLE_MULTICORE
  /**
   * @brief Sets the partitioner for parallelization.
   * @param partitioner
   */
  void setPartitioner(const tbb::auto_partitioner& partitioner);
#endif

  /**
   * @brief Runs the data algorithm.  Parallelization is used if appropriate.
   * @param body
   */
  template <typename Body>
  void execute(const Body& body)
  {
    bool doParallel = false;
#ifdef COMPLEX_ENABLE_MULTICORE
    doParallel = m_RunParallel;
    if(doParallel)
    {
      tbb::blocked_range2d<size_t, size_t> tbbRange(m_Range.minRow(), m_Range.maxRow(), m_Range.minCol(), m_Range.maxCol());
      tbb::parallel_for(tbbRange, body, m_Partitioner);
    }
#endif

    // Run non-parallel operation
    if(!doParallel)
    {
      body(m_Range);
    }
  }

private:
  RangeType m_Range;
  bool m_RunParallel = false;
#ifdef COMPLEX_ENABLE_MULTICORE
  tbb::auto_partitioner m_Partitioner;
#endif
};

} // namespace complex
