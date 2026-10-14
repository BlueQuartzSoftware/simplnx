#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageIO/ImageStackCropping.hpp"

#include <fmt/format.h>

#include <cmath>

namespace nx::core
{
/**
 * @struct ZRange
 * @brief Stores inclusive slice indices in the full input file list.
 */
struct ZRange
{
  usize zMin;
  usize zMax;
};

/**
 * @brief Computes the same full-stack slice range for preflight and execution.
 * @param croppingOptions Specifies voxel or physical Z bounds.
 * @param zDim Gives the number of input files before cropping.
 * @param origin Specifies the origin override.
 * @param spacing Specifies the spacing override.
 * @param shouldChangeOrigin Enables the origin override.
 * @param shouldChangeSpacing Enables the spacing override.
 * @param originSpacingProcessing Selects whether overrides apply before cropping.
 * @return Inclusive slice indices and bounds-clamping warnings, or an error for an invalid crop.
 */
inline Result<ZRange> ComputeCroppedZRange(const CropGeometryParameter::CropValues& croppingOptions, usize zDim, const VectorFloat32Parameter::ValueType& origin,
                                           const VectorFloat32Parameter::ValueType& spacing, bool shouldChangeOrigin, bool shouldChangeSpacing, OriginSpacingProcessing originSpacingProcessing)
{
  if(zDim == 0)
  {
    return MakeErrorResult<ZRange>(-64511, "Input file list is empty; nothing to read.");
  }

  ZRange range{0, zDim - 1};
  std::vector<Warning> warnings;
  if(croppingOptions.cropZ && croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume)
  {
    range = {static_cast<usize>(croppingOptions.zBoundVoxels[0]), static_cast<usize>(croppingOptions.zBoundVoxels[1])};
  }
  else if(croppingOptions.cropZ && croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume)
  {
    auto dimensionResult = ComputeCroppedZDimension(croppingOptions, zDim, origin, spacing, shouldChangeOrigin, shouldChangeSpacing, originSpacingProcessing);
    if(dimensionResult.invalid())
    {
      return ConvertInvalidResult<ZRange>(std::move(dimensionResult));
    }

    // Physical bounds refer to the full stack before cropping or deferred spatial overrides.
    const float64 originZ = (shouldChangeOrigin && originSpacingProcessing == OriginSpacingProcessing::Preprocessed) ? origin[2] : 0;
    const float64 spacingZ = (shouldChangeSpacing && originSpacingProcessing == OriginSpacingProcessing::Preprocessed) ? spacing[2] : 1;
    range.zMin = ComputePhysicalZMinimumIndex(croppingOptions.zBoundPhysical[0], originZ, spacingZ);
    range.zMax = range.zMin + dimensionResult.value() - 1;
    warnings = std::move(dimensionResult.warnings());
  }

  if(range.zMin > range.zMax || range.zMax >= zDim)
  {
    return MakeErrorResult<ZRange>(-64514, fmt::format("Computed Z slice range [{}, {}] is invalid for {} input files.", range.zMin, range.zMax, zDim));
  }
  return {range, std::move(warnings)};
}
} // namespace nx::core
