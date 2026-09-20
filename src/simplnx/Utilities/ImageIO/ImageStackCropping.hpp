#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOEnums.hpp"

#include <fmt/format.h>

#include <cmath>

namespace nx::core
{

/**
 * @brief Computes the Z dimension an image stack has once its Z cropping is applied.
 *
 * Voxel cropping treats the configured Z bounds as inclusive slice indices. Physical cropping
 * converts the configured Z bounds to slice indices using the Z origin and spacing that apply
 * before cropping, which are the filter's overrides only when the user asked for overrides and
 * chose to have them applied first; otherwise an origin of 0 and a spacing of 1 are assumed.
 *
 * The origin and spacing containers are templated because the image-stack filters expose these
 * parameters at different precisions. Each filter therefore keeps the arithmetic precision of its
 * own parameters rather than being silently widened or narrowed by this helper.
 *
 * @tparam VectorT Container of the origin and spacing components, indexable and at least 3 long
 * @param croppingOptions Cropping configuration taken from the filter's arguments
 * @param zDim Uncropped Z dimension, normally the number of input files
 * @param origin Image origin override
 * @param spacing Image spacing override
 * @param shouldChangeOrigin Whether the filter was asked to override the origin
 * @param shouldChangeSpacing Whether the filter was asked to override the spacing
 * @param originSpacingProcessing Whether overrides apply before or after cropping
 * @return Cropped Z dimension, or an error describing why the configured Z range is unusable
 */
template <class VectorT>
Result<usize> ComputeCroppedZDimension(const CropGeometryParameter::CropValues& croppingOptions, usize zDim, const VectorT& origin, const VectorT& spacing, bool shouldChangeOrigin,
                                       bool shouldChangeSpacing, OriginSpacingProcessing originSpacingProcessing)
{
  if(!croppingOptions.cropZ)
  {
    return {zDim};
  }

  if(croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume)
  {
    // Voxel-based Z cropping: zBoundVoxels are inclusive indices
    const auto zMin = static_cast<usize>(croppingOptions.zBoundVoxels[0]);
    const auto zMax = static_cast<usize>(croppingOptions.zBoundVoxels[1]);
    if(zMax >= zMin)
    {
      zDim = zMax - zMin + 1;
    }
    return {zDim};
  }

  if(croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume)
  {
    const float64 zMinPhys = croppingOptions.zBoundPhysical[0];
    const float64 zMaxPhys = croppingOptions.zBoundPhysical[1];

    const float64 originZ = (shouldChangeOrigin && originSpacingProcessing == OriginSpacingProcessing::Preprocessed) ? origin[2] : 0;
    const float64 spacingZ = (shouldChangeSpacing && originSpacingProcessing == OriginSpacingProcessing::Preprocessed) ? spacing[2] : 1;

    if(zMaxPhys < zMinPhys)
    {
      return MakeErrorResult<usize>(-23520, "Invalid Z cropping range: the maximum physical Z value is smaller than the minimum. Please ensure the start Z is less than or equal to the end Z.");
    }

    if(spacingZ <= 0)
    {
      return MakeErrorResult<usize>(-23521, fmt::format("Invalid Z spacing ({}). The Z spacing must be greater than zero to apply physical cropping.", spacingZ));
    }

    if(zMinPhys < originZ || zMinPhys > (static_cast<float32>(zDim) * spacingZ + originZ))
    {
      return MakeErrorResult<usize>(-23522, fmt::format("The minimum Z cropping value ({}) is outside the image bounds. Valid Z range is [{} to {}] in physical units.", zMinPhys, originZ,
                                                        (static_cast<float32>(zDim) * spacingZ + originZ)));
    }

    if(zMaxPhys < originZ || zMaxPhys > (static_cast<float32>(zDim) * spacingZ + originZ))
    {
      return MakeErrorResult<usize>(-23523, fmt::format("The maximum Z cropping value ({}) is outside the image bounds. Valid Z range is [{} to {}] in physical units.", zMaxPhys, originZ,
                                                        (static_cast<float32>(zDim) * spacingZ + originZ)));
    }

    const auto zMinIndex = static_cast<usize>(std::floor((zMinPhys - originZ) / spacingZ));
    if(zMinIndex >= zDim)
    {
      return MakeErrorResult<usize>(
          -23524, fmt::format("The minimum Z cropping value ({}) converts to slice index {} which is outside the valid slice index range [0 to {}].", zMinPhys, zMinIndex, (zDim > 0 ? zDim - 1 : 0)));
    }

    const auto zMaxIndex = static_cast<usize>(std::floor((zMaxPhys - originZ) / spacingZ));
    if(zMaxIndex >= zDim)
    {
      return MakeErrorResult<usize>(
          -23525, fmt::format("The maximum Z cropping value ({}) converts to slice index {} which is outside the valid slice index range [0 to {}].", zMaxPhys, zMaxIndex, (zDim > 0 ? zDim - 1 : 0)));
    }

    zDim = zMaxIndex - zMinIndex + 1;
  }

  return {zDim};
}

} // namespace nx::core
