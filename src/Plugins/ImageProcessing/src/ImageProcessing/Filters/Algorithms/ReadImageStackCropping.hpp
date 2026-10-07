#pragma once

#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageIO/ImageStackCropping.hpp"

namespace
{
/**
 * @struct ZRange
 * @brief Stores inclusive slice indices in the full input file list.
 */
struct ZRange
{
  nx::core::usize zMin;
  nx::core::usize zMax;
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
 * @return Inclusive slice indices, or an error for an invalid crop range.
 */
inline nx::core::Result<ZRange> ComputeCroppedZRange(const nx::core::CropGeometryParameter::CropValues& croppingOptions, nx::core::usize zDim,
                                                     const nx::core::VectorFloat32Parameter::ValueType& origin, const nx::core::VectorFloat32Parameter::ValueType& spacing, bool shouldChangeOrigin,
                                                     bool shouldChangeSpacing, nx::core::OriginSpacingProcessing originSpacingProcessing)
{
  using namespace nx::core;
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
} // namespace
