#pragma once

#include "complex/complex_export.hpp"

#include <array>
#include <cstddef>

#ifdef COMPLEX_ENABLE_MULTICORE
#include <tbb/blocked_range3d.h>
#endif
namespace complex
{

/**
 * @class ComplexRange3D ComplexRange3D.h complex/Common/ComplexRange3D.h
 * @brief The ComplexRange3D class defines a range between set of minimum and
 * maximum values. The purpose of this class is mainly to allow a more unified
 * control flow during parallelization between builds using TBB and those that
 * do not.  Because tbb::blocked_range is used in an implicit conversion constructor,
 * a single operator accepting a ComplexRange can be used TBB parallelized and
 * non-paralleled versions without a branching code base.
 */
class COMPLEX_EXPORT ComplexRange3D
{
public:
  using RangeType = std::array<size_t, 6>; //<* {size_t xMin, size_t xMax, size_t yMin, size_t yMax, size_t zMin, size_t zMax}
  using DimensionRange = std::array<size_t, 2>;

  ComplexRange3D();
  /**
   * @brief
   * @param x
   * @param y
   * @param z
   */
  ComplexRange3D(size_t x, size_t y, size_t z);
  /**
   * @brief Creates a 3D Range to use for iterate over 3D data. Note that in this
   * context the X is the fastest moving axis, then Y, then Z.
   * @param xMin
   * @param xMax
   * @param yMin
   * @param yMax
   * @param zMin
   * @param zMax
   */
  ComplexRange3D(size_t xMin, size_t xMax, size_t yMin, size_t yMax, size_t zMin, size_t zMax);
#ifdef COMPLEX_ENABLE_MULTICORE
  ComplexRange3D(const tbb::blocked_range3d<size_t>& r);
#endif

  /**
   * @brief Returns an array representation of the range.
   * @return
   */
  RangeType getRange() const;

  /**
   * @brief Returns the range along the X dimension
   * @return
   */
  DimensionRange getXRange() const;

  /**
   * @brief Returns the range along the Y dimension
   * @return
   */
  DimensionRange getYRange() const;

  /**
   * @brief Returns the range along the Z dimension
   * @return
   */
  DimensionRange getZRange() const;

  /**
   * @brief Returns true if the range is empty.  Returns false otherwise.
   * @return
   */
  bool empty() const;

  /**
   * @brief Returns the specified part of the range.  The range is organized as
   * [xMin, xMax, yMin, yMax, zMin, zMax].
   * @param index
   * @return
   */
  size_t operator[](size_t index) const;

private:
  RangeType m_Range;
};

} // namespace complex
