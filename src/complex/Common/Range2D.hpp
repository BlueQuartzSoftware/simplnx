#pragma once

#include "complex/complex_export.hpp"

#include <array>
#include <cstddef>

#ifdef COMPLEX_ENABLE_MULTICORE
#include <tbb/blocked_range2d.h>
#endif
namespace complex
{
/**
 * @class Range2D Range2D.h complex/Common/Range2D.h
 * @brief The Range2D class defines a range between set of minimum and
 * maximum values. The purpose of this class is mainly to allow a more unified
 * control flow during parallelization between builds using TBB and those that
 * do not.  Because tbb::blocked_range is used in an implicit conversion constructor,
 * a single operator accepting a Range can be used TBB parallelized and
 * non-paralleled versions without a branching code base.
 */
class COMPLEX_EXPORT Range2D
{
public:
  using RangeType = std::array<size_t, 4>; // {  col Start, col end, row start, row end }

  Range2D();

  /**
   * @brief Creates a 2D Range where X is the fastest moving axis, and then Y
   * @param yStart
   * @param yEnd
   * @param xStart
   * @param xEnd
   */
  Range2D(size_t xStart, size_t xEnd, size_t yStart, size_t yEnd);

#ifdef COMPLEX_ENABLE_MULTICORE
  /**
   * @brief Uses the tbb::blocked_rang2d<T,T> as the init value
   * @param r
   */
  Range2D(const tbb::blocked_range2d<size_t, size_t>& r);
#endif

  /**
   * @brief Returns an array representation of the range.
   * @return
   */
  RangeType getRange() const;

  /**
   * @brief Returns the minimum row index in the range.
   * @return
   */
  size_t minRow() const;

  /**
   * @brief Returns the minimum column index in the range.
   * @return
   */
  size_t minCol() const;

  /**
   * @brief Returns the maximum row index in the range.
   * @return
   */
  size_t maxRow() const;

  /**
   * @brief Returns the maximum column index in the range.
   * @return
   */
  size_t maxCol() const;

  /**
   * @brief Returns the number of rows in the range.
   * @return
   */
  size_t numRows() const;

  /**
   * @brief Returns the number of columns in the range.
   * @return
   */
  size_t numCols() const;

  /**
   * @brief Returns the number of indices in the range.
   * @return
   */
  size_t size() const;

  /**
   * @brief Returns true if the range is empty.  Returns false otherwise.
   * @return
   */
  bool empty() const;

  /**
   * @brief Returns the range based on the specified index.  The range is
   * organized as [min, max]
   */
  size_t operator[](size_t index) const;

private:
  RangeType m_Range;
};

} // namespace complex
