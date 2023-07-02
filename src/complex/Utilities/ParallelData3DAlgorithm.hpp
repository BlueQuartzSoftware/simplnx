#pragma once

#include "complex/Common/Range3D.hpp"
#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

#ifdef COMPLEX_ENABLE_MULTICORE
#include <tbb/blocked_range3d.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>
#endif

#include <array>
#include <cstddef>
#include <optional>

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

  ParallelData3DAlgorithm(const ParallelData3DAlgorithm&) = default;
  ParallelData3DAlgorithm(ParallelData3DAlgorithm&&) noexcept = default;
  ParallelData3DAlgorithm& operator=(const ParallelData3DAlgorithm&) = default;
  ParallelData3DAlgorithm& operator=(ParallelData3DAlgorithm&&) noexcept = default;

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
   * @brief Returns the optional chunk size to operate over.
   * @return optional chunk size
   */
  std::optional<RangeType> getChunkSize() const;

  /**
   * @brief Sets the preferred chunk size, if any, that should be used to operating over.
   * @param chunkSize Optional chunk size used to control how the execution algorithm picks indices to operate over.
   */
  void setChunkSize(std::optional<RangeType> chunkSize);

  /**
   * @brief Runs the data algorithm.  Parallelization is used if appropriate.
   * @param body
   */
  template <typename Body>
  void execute(const Body& body)
  {
    // Check if pre-existing chunk sizes should be preserved
    if(m_ChunkSize.has_value() == false)
    {
      executeRange<Body>(body, m_Range);
    }
    else
    {
      // Execute over pre-existing chunks

      // Get chunk size
      const usize chunkWidth = m_ChunkSize->getXRange()[1];
      const usize chunkHeight = m_ChunkSize->getYRange()[1];
      const usize chunkDepth = m_ChunkSize->getZRange()[1];

      // Check which chunks to operate over
      const auto rangeX = m_Range.getXRange();
      const auto rangeY = m_Range.getYRange();
      const auto rangeZ = m_Range.getZRange();

      const usize minChunkCol = m_Range.getXRange()[0] / chunkWidth;
      const usize maxChunkCol = m_Range.getXRange()[1] / chunkWidth;
      const usize minChunkRow = m_Range.getYRange()[0] / chunkHeight;
      const usize maxChunkRow = m_Range.getYRange()[1] / chunkHeight;
      const usize minChunkDepth = m_Range.getZRange()[0] / chunkDepth;
      const usize maxChunkDepth = m_Range.getZRange()[1] / chunkDepth;

      for(usize chunkX = minChunkCol; chunkX <= maxChunkCol; chunkX++)
      {
        const usize minX = std::max(rangeX[0], (chunkX - 1) * chunkWidth);
        const usize maxX = std::min(rangeX[1], chunkX * chunkWidth);

        for(usize chunkY = minChunkRow; chunkY <= maxChunkRow; chunkY++)
        {
          const usize minY = std::max(rangeY[0], (chunkY - 1) * chunkHeight);
          const usize maxY = std::min(rangeY[1], chunkY * chunkHeight);

          for(usize chunkZ = minChunkDepth; chunkZ <= maxChunkDepth; chunkZ++)
          {
            const usize minZ = std::max(rangeZ[0], (chunkZ - 1) * chunkDepth);
            const usize maxZ = std::min(rangeZ[1], chunkZ * chunkDepth);

            const RangeType chunkRange(minX, maxX, minY, maxY, minZ, maxZ);
            executeRange<Body>(body, chunkRange);
          }
        }
      }
    }
  }

protected:
  template <typename Body>
  void executeRange(const Body& body, const RangeType& range)
  {
#ifdef COMPLEX_ENABLE_MULTICORE
    if(m_RunParallel)
    {
      tbb::auto_partitioner partitioner;
      tbb::blocked_range3d<size_t, size_t, size_t> tbbRange(range[4], range[5], range[2], range[3], range[0], range[1]);
      tbb::parallel_for(tbbRange, body, partitioner);
    }
    else
#endif
    // Run non-parallel operation
    {
      body(range);
    }
  }

private:
  RangeType m_Range;
  std::optional<RangeType> m_ChunkSize;
#ifdef COMPLEX_ENABLE_MULTICORE
  bool m_RunParallel = true;
#else
  bool m_RunParallel = false;
#endif
};
} // namespace complex
