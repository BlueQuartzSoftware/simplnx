#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include <array>
#include <cmath>
#include <limits>

namespace nx::core::DBSCANCommon
{
inline constexpr int32 k_GridAxisTooLargeError = -54091;      // One axis needs more grid cells than a grid index can hold.
inline constexpr int32 k_GridCellCountTooLargeError = -54092; // The dense resident grid needs more cells than memory can address.
inline constexpr int32 k_NonFiniteCoordinateError = -54093;   // An unmasked coordinate is NaN or infinite.

/**
 * @brief Largest number of grid cells that one axis may span.
 *
 * Each axis of the dense grid has at least two cells, so a longer axis would
 * make the cell count of a two- or three-dimensional grid overflow usize. The
 * limit also leaves room for the neighbor offsets added to a grid coordinate.
 */
inline constexpr float64 k_MaxAxisCellSpan = static_cast<float64>(std::numeric_limits<usize>::max() / 2);

/**
 * @brief Returns the label of a coordinate axis.
 * @param axis Axis index: 0, 1 or 2.
 * @return "X", "Y" or "Z".
 */
inline char AxisLabel(usize axis)
{
  constexpr std::array<char, 3> k_Labels = {'X', 'Y', 'Z'};
  return k_Labels[axis];
}

/**
 * @brief Creates the error for an unmasked coordinate that is NaN or infinite.
 * @param arrayPath Path of the clustering array.
 * @param tupleIndex Index of the tuple in the clustering array.
 * @param axis Axis index of the value.
 * @param value The value, converted to float32 as the grid uses it.
 * @return An error with k_NonFiniteCoordinateError.
 */
inline Result<> MakeNonFiniteCoordinateError(const DataPath& arrayPath, usize tupleIndex, usize axis, float32 value)
{
  return MakeErrorResult(k_NonFiniteCoordinateError,
                         fmt::format("Tuple {} of the clustering array '{}' has the value {} on the {} axis. DBSCAN needs every unmasked coordinate to be a finite 32-bit floating-point number. "
                                     "Exclude this tuple with a mask, or replace the value.",
                                     tupleIndex, arrayPath.toString(), value, AxisLabel(axis)));
}

/**
 * @brief Checks that every coordinate of one unmasked tuple is finite.
 * @tparam N Number of coordinate axes (2 or 3).
 * @param arrayPath Path of the clustering array, named in the error message.
 * @param tupleIndex Index of the tuple in the clustering array.
 * @param coordinates The tuple's coordinates, converted to float32 as the grid uses them.
 * @return Success, or k_NonFiniteCoordinateError for the first axis whose value is NaN or infinite.
 *
 * The grid bins float32 coordinates, so a float64 value outside the float32
 * range also fails as infinite. Callers check the mask first.
 */
template <usize N>
Result<> CheckFiniteCoordinates(const DataPath& arrayPath, usize tupleIndex, const std::array<float32, N>& coordinates)
{
  for(usize axis = 0; axis < N; axis++)
  {
    if(!std::isfinite(coordinates[axis]))
    {
      return MakeNonFiniteCoordinateError(arrayPath, tupleIndex, axis, coordinates[axis]);
    }
  }
  return {};
}

/**
 * @brief Computes how many grid cells one axis spans from the grid origin to its largest binned coordinate.
 * @param arrayPath Path of the clustering array, named in the error message.
 * @param axis Axis index, named in the error message.
 * @param minimum Smallest unmasked coordinate on this axis; the grid origin is one cell below it.
 * @param maximum Largest unmasked coordinate on this axis.
 * @param sideLength Grid cell side length.
 * @param padMaximum True when the grid also extends one cell past the maximum, as the dense grid does.
 * @return The cell span before it is rounded down, or k_GridAxisTooLargeError when the
 * padded extent of the axis is not finite, or when the span is not finite or is not below
 * k_MaxAxisCellSpan. The two cases have different messages, because only the second one
 * is fixed by a larger Epsilon.
 *
 * The span is computed with the float32 operations of the grid construction,
 * so the check sees the value that the caller casts to an integer. A coordinate
 * between the minimum and the maximum gives a span that is not larger.
 * @pre minimum and maximum are finite, and minimum <= maximum.
 */
inline Result<float32> ComputeAxisCellSpan(const DataPath& arrayPath, usize axis, float32 minimum, float32 maximum, float32 sideLength, bool padMaximum)
{
  const float32 origin = minimum - sideLength;
  const float32 end = padMaximum ? maximum + sideLength : maximum;
  const float32 extent = end - origin;
  if(!std::isfinite(extent) || extent < 0.0F)
  {
    // The padded bounds or their difference left the float32 range, so a larger Epsilon would not help.
    return MakeErrorResult<float32>(k_GridAxisTooLargeError,
                                    fmt::format("DBSCAN cannot build its search grid for the clustering array '{}': its {} values from {} to {}, padded by one grid cell of side {}, "
                                                "exceed the 32-bit floating-point range. Rescale the coordinates and Epsilon to smaller values.",
                                                arrayPath.toString(), AxisLabel(axis), minimum, maximum, sideLength));
  }
  const float32 cellSpan = extent / sideLength;
  if(!std::isfinite(cellSpan) || static_cast<float64>(cellSpan) >= k_MaxAxisCellSpan)
  {
    return MakeErrorResult<float32>(k_GridAxisTooLargeError,
                                    fmt::format("DBSCAN cannot build its search grid for the clustering array '{}': its {} values from {} to {} span {} grid cells of side {}, but at most {} "
                                                "cells fit on one axis. Increase Epsilon.",
                                                arrayPath.toString(), AxisLabel(axis), minimum, maximum, cellSpan, sideLength, k_MaxAxisCellSpan));
  }
  return {cellSpan};
}

/**
 * @brief Computes the dimensions of the dense resident DBSCAN grid.
 * @tparam N Number of coordinate axes (2 or 3).
 * @param arrayPath Path of the clustering array, named in the error messages.
 * @param bounds Minimum coordinates in elements 0 to N-1 and maximum coordinates in elements N to 2N-1.
 * @param sideLength Grid cell side length. The grid extends one cell past each bound.
 * @return The number of cells on each axis (X first), k_GridAxisTooLargeError when one
 * axis is too long, or k_GridCellCountTooLargeError when the cell count, or the byte
 * count of one usize per cell, does not fit in usize.
 *
 * The axes use the float32 operations of the grid construction, so a grid that
 * passes has the same dimensions as before the check.
 * @pre The bounds are finite.
 */
template <usize N>
Result<std::array<usize, N>> ComputeGridDimensions(const DataPath& arrayPath, const std::array<float32, 2 * N>& bounds, float32 sideLength)
{
  std::array<usize, N> dims = {};
  for(usize axis = 0; axis < N; axis++)
  {
    Result<float32> spanResult = ComputeAxisCellSpan(arrayPath, axis, bounds[axis], bounds[axis + N], sideLength, true);
    if(spanResult.invalid())
    {
      return ConvertInvalidResult<std::array<usize, N>>(std::move(spanResult));
    }
    dims[axis] = static_cast<usize>(spanResult.value()) + 2;
  }

  // The construction keeps one usize per cell, so its byte count must fit as well.
  constexpr usize k_MaxCellCount = std::numeric_limits<usize>::max() / sizeof(usize);
  usize cellCount = 1;
  for(usize dim : dims)
  {
    if(dim > k_MaxCellCount / cellCount)
    {
      return MakeErrorResult<std::array<usize, N>>(k_GridCellCountTooLargeError,
                                                   fmt::format("DBSCAN cannot build its search grid for the clustering array '{}': a grid of {} cells needs more than {} cells, the most that "
                                                               "memory can address. Increase Epsilon.",
                                                               arrayPath.toString(), StringUtilities::formatDimensions(dims, true), k_MaxCellCount));
    }
    cellCount *= dim;
  }
  return {dims};
}
} // namespace nx::core::DBSCANCommon
