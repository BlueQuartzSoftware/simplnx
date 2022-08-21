#pragma once

#include "complex/Common/ComplexRange3D.hpp"
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
 * @brief The ParallelDataAlgorithm class handles parallelization across data-based algorithms.
 * A range is required, as well as an object with a matching function operator.  This class
 * utilizes TBB for parallelization and will fallback to non-parallelization if it is not
 * available or the parallelization is disabled.
 */
class COMPLEX_EXPORT ParallelData3DAlgorithm
{
public:
  ParallelData3DAlgorithm();
  virtual ~ParallelData3DAlgorithm();

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
  ComplexRange3D getRange() const;

  /**
   * @brief Sets the range to operate over.
   * @param range3D
   */
  void setRange(const ComplexRange3D& range);

  /**
   * @brief Sets the range to operate over.
   * @param range3D
   */
  void setRange(size_t xMax, size_t yMax, size_t zMax);

  /**
   * @brief Returns the grain size.
   * @return
   */
  size_t getGrain() const;

  /**
   * @brief Sets the grain size.
   * @param grain
   */
  void setGrain(size_t grain);

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
      tbb::blocked_range3d<size_t, size_t, size_t> tbbRange(m_Range[4], m_Range[5], m_Range[2], m_Range[3], m_Range[0], m_Range[1]);
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
  ComplexRange3D m_Range;
  size_t m_Grain = 1;
  bool m_RunParallel = false;
#ifdef COMPLEX_ENABLE_MULTICORE
  tbb::auto_partitioner m_Partitioner;
#endif
};
} // namespace complex
