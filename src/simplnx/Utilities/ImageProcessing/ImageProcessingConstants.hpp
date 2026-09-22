#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <set>

namespace nx::core::ImageProcessing
{
// Error codes (offset chosen to avoid collision with core/ITK codes).
inline constexpr int32 k_UnsupportedDataType = -8000;
inline constexpr int32 k_GeometryMismatch = -8001;
inline constexpr int32 k_NonBinaryInput = -8002;
// Shared opt-in check (PreflightImageFilter requireScalar): input must be single-component. Used by the
// grayscale reconstruction filters, whose ITK marker/geodesic operators are defined on scalar images only.
inline constexpr int32 k_NonScalarInput = -8550;
inline constexpr int32 k_RecursiveGaussianTooFewPixels = -8570;
inline constexpr int32 k_RecursiveGaussianNonPositiveSigma = -8571;
// The recursive Gaussian divides sigma by the per-axis spacing; ITK's RecursiveGaussianImageFilter::SetUp throws when
// a filtered axis's spacing is < 1e-8 ("suspiciously small"). Without this guard a zero/near-zero spacing yields NaN
// coefficients and silently NaN output.
inline constexpr int32 k_RecursiveGaussianNonPositiveSpacing = -8572;
// The central-difference gradient magnitude scales each axis derivative by 1/spacing; ITK's GradientMagnitudeImageFilter
// throws "Image spacing cannot be zero" when UseImageSpacing is on and an axis spacing is exactly zero.
inline constexpr int32 k_GradientMagnitudeZeroSpacing = -8573;
// ITK's AnisotropicDiffusionImageFilter::InitializeIteration WARNS (does not throw/clamp) when the user's Time Step
// exceeds the CFL stability bound minSpacing/2^(ImageDimension+1). Shared by the anisotropic-diffusion pair
// (GradientAnisotropicDiffusion now; CurvatureAnisotropicDiffusion later) via AppendUnstableTimeStepWarning.
inline constexpr int32 k_AnisotropicDiffusionUnstableTimeStep = -8580;
// RelabelComponent's consecutive output-label counter overflows the (possibly narrow, e.g. int8) SameAsInput output
// type when there are more surviving objects than the type can represent. Mirrors itk::RelabelComponentImageFilter::
// GenerateData's NumericTraits<OutputPixelType>::max() guard (which itkExceptionMacro-throws in the same situation).
inline constexpr int32 k_RelabelComponentTooManyObjects = -8590;

/**
 * @brief Output pixel type == input pixel type. The default OutTypeMap for the façade.
 */
template <class T>
using SameAsInput = T;

template <class T>
using AlwaysFloat64 = float64;

template <class T>
using AlwaysUInt8 = uint8;

template <class T>
using AlwaysUInt32 = uint32;

// @brief Output pixel type is always float32 (e.g. the signed distance transform), regardless of input type.
template <class T>
using AlwaysFloat32 = float32;

/**
 * @brief The 10 scalar numeric DataTypes an image filter operates on (no bool).
 */
inline const std::set<DataType>& GetScalarNumericTypes()
{
  static const std::set<DataType> types = {DataType::int8,   DataType::uint8, DataType::int16,  DataType::uint16,  DataType::int32,
                                           DataType::uint32, DataType::int64, DataType::uint64, DataType::float32, DataType::float64};
  return types;
}

/**
 * @brief The 8 integer scalar DataTypes (no bool, no float). Used by integer-only filters (e.g. Not).
 */
inline const std::set<DataType>& GetIntegerScalarTypes()
{
  static const std::set<DataType> types = {DataType::int8, DataType::uint8, DataType::int16, DataType::uint16, DataType::int32, DataType::uint32, DataType::int64, DataType::uint64};
  return types;
}

/**
 * @brief The 6 signed scalar DataTypes (signed integer + floating point; no unsigned, no bool). Matches the legacy
 *        ITK SignedScalarPixelIdTypeList used by SmoothingRecursiveGaussian. Unsigned types are excluded because the
 *        Deriche IIR has small negative overshoot that would underflow an unsigned round-trip.
 */
inline const std::set<DataType>& GetSignedScalarTypes()
{
  static const std::set<DataType> types = {DataType::int8, DataType::int16, DataType::int32, DataType::int64, DataType::float32, DataType::float64};
  return types;
}

/**
 * @brief The 2 floating-point scalar DataTypes (float32/float64). Matches the legacy ITK FloatingScalarPixelIdTypeList
 *        used by MinMaxCurvatureFlow + the anisotropic-diffusion filters (integer input is rejected).
 */
inline const std::set<DataType>& GetFloatingScalarTypes()
{
  static const std::set<DataType> types = {DataType::float32, DataType::float64};
  return types;
}

/**
 * @brief The 4 scalar DataTypes the axis-projection filters support (uint8, int16, uint16, float32).
 *        These are the pixel types the legacy ITK projection filters could actually PROCESS: the legacy
 *        GUI exposed all 10 scalar numeric types, but 6 of them errored at runtime, so restricting to
 *        these 4 matches what the legacy filters were actually able to handle (not their wider GUI list).
 */
inline const std::set<DataType>& GetProjectionScalarTypes()
{
  static const std::set<DataType> types = {DataType::uint8, DataType::int16, DataType::uint16, DataType::float32};
  return types;
}

namespace detail
{
template <template <class> class OutTypeMap>
struct MappedOutputTypeFn
{
  template <class T>
  DataType operator()() const
  {
    return GetDataType<OutTypeMap<T>>();
  }
};
} // namespace detail

/**
 * @brief Maps a runtime input DataType through the compile-time OutTypeMap to the output DataType,
 *        reusing the core ExecuteDataFunctionNoBool dispatch (no duplicated type switch).
 */
template <template <class> class OutTypeMap>
DataType GetMappedOutputType(DataType inputType)
{
  return ExecuteDataFunctionNoBool(detail::MappedOutputTypeFn<OutTypeMap>{}, inputType);
}
} // namespace nx::core::ImageProcessing
