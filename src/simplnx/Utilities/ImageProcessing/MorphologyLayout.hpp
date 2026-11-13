#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <limits>

namespace nx::core::ImageProcessing
{
inline constexpr int32 k_MorphologyPaddedExtentError = -88005;
inline constexpr int32 k_MorphologyCoordinateError = -88006;
inline constexpr int32 k_MorphologyPlaneError = -88007;
inline constexpr int32 k_MorphologyVolumeError = -88008;
inline constexpr int32 k_MorphologyByteCountError = -88009;

/**
 * @brief Checks image and Safe Border layout arithmetic without allocating image values.
 * @param dims Original X, Y and Z image extents in cells.
 * @param radius X, Y and Z kernel radii in cells.
 * @param safeBorder Includes radius padding, except on Z for a single-plane image.
 * @param elementBytes Size of one scalar image value in bytes.
 * @pre radius has exactly three components and satisfies ValidateKernelRadius().
 * @return Layout error if counts, coordinates or typed byte counts are unrepresentable.
 * @note Flat-offset optimization limits do not restrict this layout contract.
 */
inline Result<> ValidateMorphologyLayout(const SizeVec3& dims, nonstd::span<const uint32> radius, bool safeBorder, usize elementBytes)
{
  SizeVec3 paddedDims = dims;
  constexpr std::array<const char*, 3> axes = {"X", "Y", "Z"};
  for(usize axis = 0; axis < 3; ++axis)
  {
    const usize pad = safeBorder && (axis != 2 || dims[2] != 1) ? static_cast<usize>(radius[axis]) : 0;
    if(pad > (std::numeric_limits<usize>::max() - dims[axis]) / 2)
    {
      return MakeErrorResult(k_MorphologyPaddedExtentError,
                             fmt::format("Morphology dimensions ({}) and Kernel Radius (X={}, Y={}, Z={}) overflow padded extent dims[{}]+2*pad. Reduce the dimensions or radii.",
                                         StringUtilities::formatDimensions3D(dims), radius[0], radius[1], radius[2], axes.at(axis)));
    }
    paddedDims[axis] += 2 * pad;
  }
  constexpr uint64 signedMax = static_cast<uint64>(std::numeric_limits<int64>::max());
  for(usize axis = 0; axis < 3; ++axis)
  {
    // Padded extents are at least the original extents, so this also bounds original coordinates.
    if(paddedDims[axis] > signedMax - static_cast<uint64>(radius[axis]) - 1)
    {
      return MakeErrorResult(k_MorphologyCoordinateError,
                             fmt::format("Morphology dimensions ({}) and Kernel Radius (X={}, Y={}, Z={}) exceed the signed coordinate limit: padded extent {} + radius {} + 1 on axis {} must be at "
                                         "most {}. Reduce the dimensions or radii.",
                                         StringUtilities::formatDimensions3D(dims), radius[0], radius[1], radius[2], paddedDims[axis], radius[axis], axes.at(axis), signedMax));
    }
  }
  if(paddedDims[0] != 0 && paddedDims[1] > signedMax / paddedDims[0])
  {
    return MakeErrorResult(
        k_MorphologyPlaneError,
        fmt::format("Morphology dimensions ({}) and Kernel Radius (X={}, Y={}, Z={}) exceed the signed plane count limit: padded X*Y = {}*{} must be at most {}. Reduce the dimensions or radii.",
                    StringUtilities::formatDimensions3D(dims), radius[0], radius[1], radius[2], paddedDims[0], paddedDims[1], signedMax));
  }
  const uint64 plane = static_cast<uint64>(paddedDims[0]) * paddedDims[1];
  if(plane != 0 && paddedDims[2] > signedMax / plane)
  {
    return MakeErrorResult(
        k_MorphologyVolumeError,
        fmt::format("Morphology dimensions ({}) and Kernel Radius (X={}, Y={}, Z={}) exceed the signed volume count limit: padded X*Y*Z = {}*{}*{} must be at most {}. Reduce the dimensions or radii.",
                    StringUtilities::formatDimensions3D(dims), radius[0], radius[1], radius[2], paddedDims[0], paddedDims[1], paddedDims[2], signedMax));
  }
  const uint64 volume = plane * paddedDims[2];
  if(elementBytes != 0 && volume > std::numeric_limits<uint64>::max() / elementBytes)
  {
    return MakeErrorResult(
        k_MorphologyByteCountError,
        fmt::format("Morphology dimensions ({}) and Kernel Radius (X={}, Y={}, Z={}) overflow the typed byte count: padded volume {} * element bytes {}. Reduce the dimensions or radii.",
                    StringUtilities::formatDimensions3D(dims), radius[0], radius[1], radius[2], volume, elementBytes));
  }
  return {};
}

/**
 * @brief Checks the layout of a rasterized kernel before an engine allocates scratch.
 * @param dims Original image extents in cells.
 * @param kernel Rasterized kernel. Negative legacy radii describe an empty range.
 * @param safeBorder Includes the effective Safe Border padding.
 * @param elementBytes Size of one scalar value in bytes.
 * @pre Kernel radii satisfy ValidateKernelRadius() after negative legacy radii are clamped to zero.
 * @return Layout error.
 */
inline Result<> ValidateMorphologyLayout(const SizeVec3& dims, const StructuringElement& kernel, bool safeBorder, usize elementBytes)
{
  const std::array<uint32, 3> radius = {static_cast<uint32>(std::max<int32>(0, kernel.radius[0])), static_cast<uint32>(std::max<int32>(0, kernel.radius[1])),
                                        static_cast<uint32>(std::max<int32>(0, kernel.radius[2]))};
  return ValidateMorphologyLayout(dims, radius, safeBorder, elementBytes);
}

} // namespace nx::core::ImageProcessing
