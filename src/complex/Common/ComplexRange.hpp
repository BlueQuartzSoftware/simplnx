#pragma once

#include "complex/complex_export.hpp"

#ifdef COMPLEX_ENABLE_MULTICORE
#include <tbb/blocked_range.h>
#endif

#include <array>
#include <cstddef>

namespace complex
{
/**
 * @class ComplexRange ComplexRange.h complex/Common/ComplexRange.h
 * @brief The ComplexRange class defines a range between set of minimum and
 * maximum values. The purpose of this class is mainly to allow a more unified
 * control flow during parallelization between builds using TBB and those that
 * do not.  Because tbb::blocked_range is used in an implicit conversion constructor,
 * a single operator accepting a ComplexRange can be used TBB parallelized and
 * non-paralleled versions without a branching code base.
 */
class COMPLEX_EXPORT ComplexRange
{
public:
  using RangeType = std::array<size_t, 2>;

  ComplexRange();
  ComplexRange(size_t begin, size_t end);
#ifdef COMPLEX_ENABLE_MULTICORE
  ComplexRange(const tbb::blocked_range<size_t>& r);
#endif

  /**
   * @brief Returns an array representation of the range.
   * @return
   */
  RangeType getRange() const;

  /**
   * @brief Returns the minimum index in the range.
   * @return
   */
  size_t min() const;

  /**
   * @brief Returns the maximum index in the range.
   * @return
   */
  size_t max() const;

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
