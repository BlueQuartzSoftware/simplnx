#pragma once

#include "simplnx/Common/Types.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

// Windows.h (pulled in transitively on MSVC builds) defines min/max as function-like
// macros, which collide with the public member names below. Undef them so the member
// declarations parse correctly and so std::min/std::max inside the inline methods are
// not expanded.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace nx::core
{
/**
 * @brief An N-dimensional, optionally strided index region described by inclusive
 * min/max bounds and a per-dimension stride.
 *
 * Used by the out-of-core storage and visualization code to describe the region
 * of an array being read, written, or cached. The min and max vectors give the
 * inclusive lower and upper bounds in each dimension; the stride vector gives the
 * step size in each dimension (all ones for a dense region). All three vectors
 * share the same number of dimensions.
 */
struct Extent
{
  std::vector<uint64> min;    // N-dimensional lower bounds (inclusive)
  std::vector<uint64> max;    // N-dimensional upper bounds (inclusive)
  std::vector<uint64> stride; // N-dimensional step sizes (default all-ones)

  /**
   * @brief Constructs an empty (zero-dimensional) Extent.
   */
  Extent() = default;

  /**
   * @brief Constructs an Extent from min and max bound vectors with default stride (all ones).
   * Both vectors must have the same number of dimensions.
   * @param minVal N-dimensional lower bounds (inclusive)
   * @param maxVal N-dimensional upper bounds (inclusive)
   * @throw std::invalid_argument if minVal and maxVal have different sizes
   */
  Extent(std::vector<uint64> minVal, std::vector<uint64> maxVal)
  : min(std::move(minVal))
  , max(std::move(maxVal))
  , stride(min.size(), 1)
  {
    if(min.size() != max.size())
    {
      throw std::invalid_argument("Extent min and max must have the same number of dimensions");
    }
  }

  /**
   * @brief Constructs an Extent from min, max, and stride vectors.
   * All vectors must have the same number of dimensions. Each stride value
   * must be >= 1.
   * @param minVal N-dimensional lower bounds (inclusive)
   * @param maxVal N-dimensional upper bounds (inclusive)
   * @param strideVal N-dimensional step sizes (>= 1 per dimension)
   * @throw std::invalid_argument if sizes differ or any stride is zero
   */
  Extent(std::vector<uint64> minVal, std::vector<uint64> maxVal, std::vector<uint64> strideVal)
  : min(std::move(minVal))
  , max(std::move(maxVal))
  , stride(std::move(strideVal))
  {
    if(min.size() != max.size() || min.size() != stride.size())
    {
      throw std::invalid_argument("Extent min, max, and stride must have the same number of dimensions");
    }
    for(uint64 d = 0; d < stride.size(); ++d)
    {
      if(stride[d] == 0)
      {
        throw std::invalid_argument("Extent stride must be >= 1 in every dimension");
      }
    }
  }

  /**
   * @brief Returns the number of dimensions.
   */
  uint64 dimensions() const
  {
    return min.size();
  }

  /**
   * @brief Returns the number of strided elements along the given dimension.
   *
   * With stride s, the selected indices are min[d], min[d]+s, min[d]+2s, ...
   * up to max[d] (inclusive). The count is:
   *   ceil((max[d] - min[d] + 1) / stride[d])
   * which equals (max[d] - min[d] + stride[d]) / stride[d] using integer
   * arithmetic (ceiling division).
   */
  uint64 size(uint64 dim) const
  {
    uint64 span = max[dim] - min[dim] + 1;
    uint64 s = stride[dim];
    return (span + s - 1) / s;
  }

  /**
   * @brief Returns the product of all dimension sizes (accounting for stride).
   */
  uint64 totalElements() const
  {
    if(min.empty())
    {
      return 0;
    }
    uint64 total = 1;
    for(uint64 d = 0; d < dimensions(); ++d)
    {
      uint64 s = size(d);
      if(s > 0 && total > std::numeric_limits<uint64>::max() / s)
      {
        throw std::overflow_error("Extent::totalElements() overflow");
      }
      total *= s;
    }
    return total;
  }

  /**
   * @brief Returns true if this extent fully contains the other extent.
   * Containment is determined by min/max bounds only (stride-agnostic).
   */
  bool contains(const Extent& other) const
  {
    if(dimensions() != other.dimensions())
    {
      return false;
    }
    for(uint64 d = 0; d < dimensions(); ++d)
    {
      if(other.min[d] < min[d] || other.max[d] > max[d])
      {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief Returns true if this extent overlaps with the other extent.
   * Adjacent extents (touching but not sharing elements) do NOT overlap.
   * Overlap is determined by min/max bounds only (stride-agnostic).
   */
  bool overlaps(const Extent& other) const
  {
    if(dimensions() != other.dimensions())
    {
      return false;
    }
    for(uint64 d = 0; d < dimensions(); ++d)
    {
      if(max[d] < other.min[d] || other.max[d] < min[d])
      {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief Returns the intersection of this extent with the other extent.
   * If the extents do not overlap, the result is an empty (0-dimensional) extent.
   * The resulting stride in each dimension is the element-wise maximum of the
   * two input strides.
   */
  Extent intersect(const Extent& other) const
  {
    if(!overlaps(other))
    {
      return Extent{};
    }
    std::vector<uint64> newMin(dimensions());
    std::vector<uint64> newMax(dimensions());
    std::vector<uint64> newStride(dimensions());
    for(uint64 d = 0; d < dimensions(); ++d)
    {
      newMin[d] = std::max(min[d], other.min[d]);
      newMax[d] = std::min(max[d], other.max[d]);
      newStride[d] = std::max(stride[d], other.stride[d]);
    }
    return Extent{std::move(newMin), std::move(newMax), std::move(newStride)};
  }

  /**
   * @brief Returns true if all of min, max, and stride compare equal to the other extent.
   * @param other The extent to compare against
   * @return true if every member vector is element-wise equal, false otherwise
   */
  bool operator==(const Extent& other) const = default;
};
} // namespace nx::core
