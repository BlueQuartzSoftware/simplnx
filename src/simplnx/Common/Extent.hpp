#pragma once

#include "simplnx/Common/Types.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

// Windows headers can define function-like min and max macros. The macros collide
// with the public member names and standard-library calls below. Undefine them.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace nx::core
{

/**
 * @struct Extent
 * @brief Defines an inclusive, optionally strided multidimensional region.
 *
 * Out-of-core I/O and visualization use Extent to select array regions. The
 * bounds and stride vectors use the same dimension order and rank.
 */
struct Extent
{
  std::vector<uint64> min;    // Inclusive lower bound for each dimension.
  std::vector<uint64> max;    // Inclusive upper bound for each dimension.
  std::vector<uint64> stride; // Step size for each dimension. Dense extents use one.

  Extent() = default;

  /**
   * @brief Uses unit stride for every dimension.
   * @param minVal Inclusive lower bounds.
   * @param maxVal Inclusive upper bounds.
   * @throws std::invalid_argument if the bound-vector ranks differ.
   * @pre Each minVal[d] is no greater than maxVal[d].
   *
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
   * @brief Constructs a strided extent.
   * @param minVal Inclusive lower bounds.
   * @param maxVal Inclusive upper bounds.
   * @param strideVal Positive step size for each dimension.
   * @throws std::invalid_argument if vector ranks differ or a stride is zero.
   * @pre Each minVal[d] is no greater than maxVal[d].
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

  uint64 dimensions() const
  {
    return min.size();
  }

  /**
   * @brief Returns the selected index count in one dimension.
   * @param dim Zero-based dimension index.
   * @return Count from the inclusive bounds and stride.
   * @pre dim is less than dimensions().
   * @pre max[dim] is not less than min[dim].
   * @pre The span and ceiling-division arithmetic fit uint64.
   */
  uint64 size(uint64 dim) const
  {
    uint64 span = max[dim] - min[dim] + 1;
    uint64 s = stride[dim];
    return (span + s - 1) / s;
  }

  /**
   * @brief Returns the selected element count.
   * @return Zero for a zero-rank extent. Otherwise, the product of all strided
   * dimension counts.
   * @throws std::overflow_error if the product exceeds uint64.
   * @pre Each dimension fulfills size() preconditions.
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
   * @brief Tests whether this extent contains another extent's bounds.
   * @param other Extent to test.
   * @return True when every bound of other is within this extent.
   *
   * Containment ignores stride because it describes covered coordinates.
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
   * @brief Tests whether this extent shares coordinates with another extent.
   * @param other Extent to test.
   * @return True when the inclusive bounds overlap.
   *
   * Overlap ignores stride. Adjacent extents do not overlap.
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
   * @brief Returns the bounds common to this extent and another extent.
   * @param other Extent to intersect.
   * @return A zero-rank extent when the bounds do not overlap. Otherwise, the
   * bounded intersection.
   *
   * Each result stride is the larger input stride in that dimension.
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

  bool operator==(const Extent& other) const = default;
};
} // namespace nx::core
