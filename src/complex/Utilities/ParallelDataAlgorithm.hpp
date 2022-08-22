#pragma once

#include "complex/Common/Range.hpp"
#include "complex/complex_export.hpp"

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
 * @brief The ParallelDataAlgorithm class handles parallelization across data-based algorithms.
 * A range is required, as well as an object with a matching function operator.  This class
 * utilizes TBB for parallelization and will fallback to non-parallelization if it is not
 * available or the parallelization is disabled.
 */
class COMPLEX_EXPORT ParallelDataAlgorithm
{
public:
  using RangeType = Range;

  ParallelDataAlgorithm();
  virtual ~ParallelDataAlgorithm();

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
  Range getRange() const;

  /**
   * @brief Sets the range to operate over.
   * @param range2D
   */
  void setRange(const Range& range);

  /**
   * @brief Sets the range to operate over.
   * @param min
   * @param max
   */
  void setRange(size_t min, size_t max);

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
      tbb::blocked_range<size_t> tbbRange(m_Range[0], m_Range[1]);
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
