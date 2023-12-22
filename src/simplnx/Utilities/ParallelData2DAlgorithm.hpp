#pragma once

#include "simplnx/Common/Range2D.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/IParallelAlgorithm.hpp"
#include "simplnx/simplnx_export.hpp"

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>
#endif

#include <array>
#include <cstddef>
#include <optional>

namespace nx::core
{

/**
 * @brief The ParallelData2DAlgorithm class handles parallelization across 2D data-based algorithms.
 * A range is required, as well as an object with a matching function operator.  This class
 * utilizes TBB for parallelization and will fallback to non-parallelization if it is not
 * available or the parallelization is disabled.
 */
class SIMPLNX_EXPORT ParallelData2DAlgorithm : public IParallelAlgorithm
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
      const usize chunkWidth = m_ChunkSize->maxCol() - m_ChunkSize->minCol();
      const usize chunkHeight = m_ChunkSize->maxRow() - m_ChunkSize->minRow();

      // Check which chunks to operate over
      const usize minChunkCol = m_Range.minCol() / chunkWidth;
      const usize maxChunkCol = m_Range.maxCol() / chunkWidth;
      const usize minChunkRow = m_Range.minRow() / chunkHeight;
      const usize maxChunkRow = m_Range.maxRow() / chunkHeight;

      for(usize chunkX = minChunkCol; chunkX <= maxChunkCol; chunkX++)
      {
        const usize minX = std::max(m_Range.minCol(), (chunkX - 1) * m_ChunkSize->maxCol());
        const usize maxX = std::min(m_Range.maxCol(), chunkX * m_ChunkSize->maxCol());

        for(usize chunkY = minChunkRow; chunkY <= maxChunkRow; chunkY++)
        {
          const usize minY = std::max(m_Range.minRow(), (chunkY - 1) * m_ChunkSize->maxRow());
          const usize maxY = std::min(m_Range.maxRow(), chunkY * m_ChunkSize->maxRow());

          const RangeType chunkRange(minX, maxX, minY, maxY);
          executeRange<Body>(body, chunkRange);
        }
      }
    }
  }

protected:
  template <typename Body>
  void executeRange(const Body& body, const RangeType& range)
  {
#ifdef SIMPLNX_ENABLE_MULTICORE
    if(getParallelizationEnabled())
    {
      tbb::auto_partitioner partitioner;
      tbb::blocked_range2d<size_t, size_t> tbbRange(range.minRow(), range.maxRow(), range.minCol(), range.maxCol());
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
};

} // namespace nx::core
