#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/AdaptiveHistogramEqualizationEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/AxisProjectionEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/BinaryThinningEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/BoxNeighborhoodEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ConnectedComponentEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ContourEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/DanielssonDistanceMapEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/DiscreteGaussianEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/FastChamferDistanceEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/FiniteDifferenceEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/GradientMagnitudeEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/HistogramEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/IsoContourDistanceEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/MaskEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/MaurerDistanceMapEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologyEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ObjectMorphologyEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/PointwiseEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ProjectionReducers.hpp"
#include "simplnx/Utilities/ImageProcessing/RecursiveGaussianEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/RegionalExtremaEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StreamingStatistics.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"
#include "simplnx/Utilities/ImageProcessing/WatershedFromMarkersEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ZeroCrossingEngine.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/core.h>
#include <fmt/format.h>
#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/combinable.h>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
// Element-type dispatch policies. Each reuses a core ExecuteDataFunction* helper + its matching
// allowed-DataType set — no duplicated type switch.
struct AllNumeric
{
  static const std::set<DataType>& allowedTypes()
  {
    return GetScalarNumericTypes();
  }
  template <class FuncT>
  static Result<> dispatch(DataType dataType, FuncT&& func)
  {
    return ExecuteDataFunctionNoBool(std::forward<FuncT>(func), dataType);
  }
};

struct IntegerOnly
{
  static const std::set<DataType>& allowedTypes()
  {
    return GetIntegerScalarTypes();
  }
  template <class FuncT>
  static Result<> dispatch(DataType dataType, FuncT&& func)
  {
    return ExecuteDataFunctionIntType(std::forward<FuncT>(func), dataType);
  }
};

/**
 * @brief Element-type dispatch policy restricted to the 4 types the axis-projection filters support
 *        (uint8, int16, uint16, float32) -- the pixel types the legacy ITK projection filters could
 *        actually process at runtime (their GUI listed all 10 scalar types but only these 4 worked).
 *        There is no core 4-type dispatch helper, so the switch is written out here (the same shape as
 *        the other policies' dispatch()).
 */
struct ProjectionScalar
{
  static const std::set<DataType>& allowedTypes()
  {
    return GetProjectionScalarTypes();
  }
  template <class FuncT>
  static Result<> dispatch(DataType dataType, FuncT&& func)
  {
    switch(dataType)
    {
    case DataType::uint8:
      return func.template operator()<uint8>();
    case DataType::int16:
      return func.template operator()<int16>();
    case DataType::uint16:
      return func.template operator()<uint16>();
    case DataType::float32:
      return func.template operator()<float32>();
    default:
      return MakeErrorResult(k_UnsupportedDataType, fmt::format("Axis projection does not support DataType '{}'. Supported types: uint8, int16, uint16, float32.", DataTypeToString(dataType)));
    }
  }
};

struct SignedScalar
{
  static const std::set<DataType>& allowedTypes()
  {
    return GetSignedScalarTypes();
  }
  template <class FuncT>
  static Result<> dispatch(DataType dataType, FuncT&& func)
  {
    switch(dataType)
    {
    case DataType::int8:
      return func.template operator()<int8>();
    case DataType::int16:
      return func.template operator()<int16>();
    case DataType::int32:
      return func.template operator()<int32>();
    case DataType::int64:
      return func.template operator()<int64>();
    case DataType::float32:
      return func.template operator()<float32>();
    case DataType::float64:
      return func.template operator()<float64>();
    default:
      return MakeErrorResult(k_UnsupportedDataType,
                             fmt::format("SmoothingRecursiveGaussian supports only signed scalar types (int8/int16/int32/int64/float32/float64), not '{}'.", DataTypeToString(dataType)));
    }
  }
};

/**
 * @brief Element-type dispatch policy restricted to the 2 floating-point scalar types (float32/float64). Matches
 *        the legacy ITK FloatingScalarPixelIdTypeList used by MinMaxCurvatureFlow + the anisotropic-diffusion
 *        filters, whose PDE update requires a signed, unbounded-precision pixel type (integer input is rejected).
 */
struct FloatingScalar
{
  static const std::set<DataType>& allowedTypes()
  {
    return GetFloatingScalarTypes();
  }
  template <class FuncT>
  static Result<> dispatch(DataType dataType, FuncT&& func)
  {
    switch(dataType)
    {
    case DataType::float32:
      return func.template operator()<float32>();
    case DataType::float64:
      return func.template operator()<float64>();
    default:
      return MakeErrorResult(k_UnsupportedDataType, fmt::format("This filter supports only floating-point scalar types (float32/float64), not '{}'.", DataTypeToString(dataType)));
    }
  }
};

namespace detail
{
/**
 * @brief Narrow a computed @c float64 to output element type @c U without undefined behavior. For an integer U
 *        the value is CLAMPED (saturated) to U's representable range before the cast: a pointwise filter's
 *        Float64 output bound (e.g. a default OutputMaximum of 255) can exceed a narrow output type (int8), and
 *        a direct @c static_cast<U> of an out-of-range double is UB. The exclusive power-of-two upper bound
 *        (2^(bits-1) signed / 2^bits unsigned, via std::ldexp) makes the range test exact even at 64-bit widths,
 *        and NaN maps to 0 (no meaningful integer). For a floating-point U any finite double casts by ordinary
 *        rounding (out-of-range -> +/-inf), so no clamp is applied.
 */
template <class U>
inline U SaturateCastFromDouble(float64 v)
{
  if constexpr(std::is_integral_v<U>)
  {
    if(std::isnan(v))
    {
      return U{0};
    }
    constexpr int k_Bits = 8 * static_cast<int>(sizeof(U));
    const float64 lowestExact = static_cast<float64>(std::numeric_limits<U>::lowest());
    const float64 upperExclusive = std::is_signed_v<U> ? std::ldexp(1.0, k_Bits - 1) : std::ldexp(1.0, k_Bits);
    if(v < lowestExact)
    {
      return std::numeric_limits<U>::lowest();
    }
    if(v >= upperExclusive)
    {
      return std::numeric_limits<U>::max();
    }
    return static_cast<U>(v);
  }
  else
  {
    return static_cast<U>(v);
  }
}

/**
 * @brief Validate a single Float64 @p value that will be narrowed to an INTEGER pixel type (element-type name
 *        @p typeName, representable range [@p lowest, @p highest]): a non-finite value is rejected with
 *        @p nonFiniteCode, an out-of-range value with @p outOfRangeCode (both are UNDEFINED float->integral
 *        conversions), and a value with a fractional part (representable, truncates toward zero) yields a
 *        @p truncatedWarnCode warning. @p label is the human parameter name used in the messages (e.g.
 *        "Background Value"). This is the single source of the per-value checks + message strings, shared by
 *        @ref BinaryFgBgRangeGuard (once per parameter) and @ref SingleValueRangeGuard.
 */
template <class T>
inline Result<> ValidateOneValueInRange(float64 value, const char* label, int32 nonFiniteCode, int32 outOfRangeCode, int32 truncatedWarnCode)
{
  static_assert(std::is_integral_v<T>, "ValidateOneValueInRange handles integer element types only (float types use the float branch of ScalarValueRangeGuard).");
  const std::string typeName = DataTypeToString(GetDataType<T>());
  Result<> result;
  if(!std::isfinite(value))
  {
    return MakeErrorResult(nonFiniteCode, fmt::format("{} must be a finite number for the integer input image type '{}', but is {}.", label, typeName, value));
  }
  const float64 truncated = std::trunc(value);
  // A finite, integer-valued float64 is a well-defined static_cast<T> iff lowest<T> <= truncated <= max<T> as
  // REAL numbers. lowest<T> is exactly representable in float64 at every integer width, but max<T> is NOT for
  // (u)int64: (float64)INT64_MAX rounds UP to 2^63, so the old `truncated > (float64)max<T>` test ACCEPTED
  // 2^63 and then static_cast<int64>(2^63) was undefined behavior. Compare instead against the EXCLUSIVE
  // power-of-two upper bound (2^(bits-1) signed / 2^bits unsigned), which std::ldexp yields EXACTLY at every
  // width; there is no representable double strictly between the largest in-range value and that bound, so no
  // valid value is ever wrongly rejected.
  constexpr int k_Bits = 8 * static_cast<int>(sizeof(T));
  const float64 lowestExact = static_cast<float64>(std::numeric_limits<T>::lowest());
  const float64 upperExclusive = std::is_signed_v<T> ? std::ldexp(1.0, k_Bits - 1) : std::ldexp(1.0, k_Bits);
  if(truncated < lowestExact || truncated >= upperExclusive)
  {
    return MakeErrorResult(outOfRangeCode,
                           fmt::format("{} ({}) is outside the representable range of the integer input image type '{}' and would be an undefined conversion.", label, value, typeName));
  }
  if(truncated != value)
  {
    result.warnings().push_back(Warning{truncatedWarnCode, fmt::format("{} ({}) will be truncated to {} to match the integer input image type '{}'.", label, value, truncated, typeName)});
  }
  return result;
}

/**
 * @brief Type-dispatched body of @ref ValidateBinaryFgBgInRange: validates the Float64 foreground/background
 *        parameters against an INTEGER input element type T by delegating each to @ref ValidateOneValueInRange.
 *        Returns early on the FIRST parameter that errors (discarding any warning already accumulated, matching
 *        the pre-refactor behavior) and otherwise accumulates the per-parameter truncation warnings in order.
 */
struct BinaryFgBgRangeGuard
{
  float64 foregroundValue;
  float64 backgroundValue;
  int32 nonFiniteCode;
  int32 outOfRangeCode;
  int32 truncatedWarnCode;

  template <class T>
  Result<> operator()() const
  {
    Result<> result;
    const std::array<std::pair<const char*, float64>, 2> params{{{"Foreground Value", foregroundValue}, {"Background Value", backgroundValue}}};
    for(const auto& [label, value] : params)
    {
      Result<> valueResult = ValidateOneValueInRange<T>(value, label, nonFiniteCode, outOfRangeCode, truncatedWarnCode);
      if(valueResult.invalid())
      {
        return valueResult;
      }
      for(Warning& warning : valueResult.warnings())
      {
        result.warnings().push_back(std::move(warning));
      }
    }
    return result;
  }
};

/**
 * @brief Type-dispatched body of @ref ValidateBackgroundInRange: validates a SINGLE Float64 value (labeled by
 *        @c label, e.g. "Background Value") against an INTEGER input element type T by delegating to the shared
 *        @ref ValidateOneValueInRange primitive. Used by contour filters that expose only a Background Value
 *        (no Foreground), so the error text names the right parameter.
 */
struct SingleValueRangeGuard
{
  float64 value;
  const char* label;
  int32 nonFiniteCode;
  int32 outOfRangeCode;
  int32 truncatedWarnCode;

  template <class T>
  Result<> operator()() const
  {
    return ValidateOneValueInRange<T>(value, label, nonFiniteCode, outOfRangeCode, truncatedWarnCode);
  }
};

/**
 * @brief Type-dispatched body of @ref ValidateScalarValueForType: validates a SINGLE Float64 @p value (labeled
 *        by @p label, e.g. "Object Value") that will be cast to the input element type T, which may be INTEGER
 *        OR FLOAT. For an integer T it delegates to the shared @ref ValidateOneValueInRange primitive
 *        (non-finite -> error, out-of-range -> error, fractional -> warning). For a float T any FINITE value is
 *        representable (ordinary float32 rounding), so only a non-finite value is rejected -- a NaN/Inf would
 *        break the exact-equality object test the object-morphology engine performs. Unlike @ref
 *        SingleValueRangeGuard (integer-only, used by contour filters) this guard handles all 10 scalar types,
 *        which object morphology accepts.
 */
struct ScalarValueRangeGuard
{
  float64 value;
  const char* label;
  int32 nonFiniteCode;
  int32 outOfRangeCode;
  int32 truncatedWarnCode;

  template <class T>
  Result<> operator()() const
  {
    if constexpr(std::is_integral_v<T>)
    {
      return ValidateOneValueInRange<T>(value, label, nonFiniteCode, outOfRangeCode, truncatedWarnCode);
    }
    else
    {
      // Float element type: a non-finite value would break the exact-equality object test, so reject it.
      // A finite float64 can still overflow a narrower float element type (e.g. 1e300 -> float32 +inf), which
      // would silently make the exact-equality test match nothing, so also reject anything outside the type's
      // representable magnitude. There is no fractional-truncation concern (float assignment rounds, it does
      // not truncate toward zero), so no truncation warning is emitted.
      if(!std::isfinite(value))
      {
        return MakeErrorResult(nonFiniteCode, fmt::format("{} must be a finite number, but is {}.", label, value));
      }
      const auto highest = static_cast<float64>(std::numeric_limits<T>::max());
      if(value < -highest || value > highest) // float types are symmetric: lowest() == -max()
      {
        return MakeErrorResult(outOfRangeCode, fmt::format("{} ({}) is outside the representable range [{}, {}] of the floating-point input image type '{}'.", label, value, -highest, highest,
                                                           DataTypeToString(GetDataType<T>())));
      }
      return {};
    }
  }
};
} // namespace detail

/**
 * @brief Validates the Float64 foreground/background parameters of a binary morphology filter against the
 *        (integer) input element type @p inputType, dispatching through the @ref IntegerOnly policy.
 *
 * The binary morphology façade casts foreground/background to the input element type, which is UNDEFINED for a
 * non-finite or out-of-range value on an integral type; this rejects those up front (returning an error with
 * @p nonFiniteCode / @p outOfRangeCode) and warns (via @p truncatedWarnCode) when a value has a fractional part
 * that will be truncated. The three error/warning codes are supplied by the caller so each filter keeps its own
 * unique codes. Shared by the Binary Dilate/Erode filters (and Task-5 binary composites); @p inputType must be
 * an integer scalar type (the callers guarantee this via PreflightImageFilter<IntegerOnly>).
 */
inline Result<> ValidateBinaryFgBgInRange(DataType inputType, float64 foreground, float64 background, int32 nonFiniteCode, int32 outOfRangeCode, int32 truncatedWarnCode)
{
  return IntegerOnly::dispatch(inputType, detail::BinaryFgBgRangeGuard{foreground, background, nonFiniteCode, outOfRangeCode, truncatedWarnCode});
}

/**
 * @brief Validates a single Float64 Background Value against the (integer) input element type @p inputType,
 *        dispatching through the @ref IntegerOnly policy. The label-contour façade casts the background to the
 *        input element type, which is UNDEFINED for a non-finite or out-of-range value on an integral type; this
 *        rejects those up front (@p nonFiniteCode / @p outOfRangeCode) and warns (@p truncatedWarnCode) when the
 *        value has a fractional part that will be truncated. This is the single-value analogue of
 *        @ref ValidateBinaryFgBgInRange for a filter that exposes only a Background Value (no Foreground), so the
 *        error message names "Background Value" rather than borrowing the foreground label. @p inputType must be
 *        an integer scalar type (the caller guarantees this via PreflightImageFilter<IntegerOnly>).
 */
inline Result<> ValidateBackgroundInRange(DataType inputType, float64 background, int32 nonFiniteCode, int32 outOfRangeCode, int32 truncatedWarnCode)
{
  return IntegerOnly::dispatch(inputType, detail::SingleValueRangeGuard{background, "Background Value", nonFiniteCode, outOfRangeCode, truncatedWarnCode});
}

/**
 * @brief The recursive-Gaussian filters require at least 4 pixels along every FILTERED axis (ITK's
 *        RecursiveSeparableImageFilter throws otherwise). A Z size of 1 is treated as a 2D image (the ITK bridge does
 *        the same), so Z is only required to be >= 4 when Z > 1.
 */
inline Result<> ValidateSeparableImageDims(const SizeVec3& dims, int32 errorCode)
{
  const bool zIs2D = dims[2] == 1;
  const bool ok = dims[0] >= 4 && dims[1] >= 4 && (zIs2D || dims[2] >= 4);
  if(!ok)
  {
    return MakeErrorResult(errorCode,
                           fmt::format("The recursive Gaussian requires at least 4 pixels along each filtered axis (got {}x{}x{}; a Z size of 1 is treated as 2D).", dims[0], dims[1], dims[2]));
  }
  return {};
}

/**
 * @brief The recursive Gaussian requires a strictly positive sigma (ITK's RecursiveGaussianImageFilter throws for
 *        sigma <= 0; sigma/spacing feeds sin/exp/division). Scalar overload for the single-sigma filters.
 */
inline Result<> ValidatePositiveSigma(float64 sigma, int32 errorCode)
{
  if(!(sigma > 0.0))
  {
    return MakeErrorResult(errorCode, fmt::format("Sigma must be greater than zero (got {}).", sigma));
  }
  return {};
}

/**
 * @brief Per-axis overload (SmoothingRecursiveGaussian). Only the sigmas for FILTERED axes are required to be
 *        positive: a Z size of 1 is a 2D image, so sigma[2] is unused there.
 */
inline Result<> ValidatePositiveSigma(const std::vector<float64>& sigma, const SizeVec3& dims, int32 errorCode)
{
  const uint32 effDim = (dims[2] > 1) ? 3u : 2u;
  for(uint32 a = 0; a < effDim; ++a)
  {
    if(a >= sigma.size() || !(sigma[a] > 0.0))
    {
      return MakeErrorResult(errorCode, fmt::format("Sigma for axis {} must be greater than zero.", a));
    }
  }
  return {};
}

/**
 * @brief Validates that every FILTERED axis has a non-degenerate image spacing. The Gaussian derivative divides by the
 *        per-axis spacing, so a zero/near-zero spacing produces NaN/inf coefficients and silently non-finite output.
 *        This replicates ITK's own guards: RecursiveGaussianImageFilter::SetUp throws for spacing < 1e-8 ("suspiciously
 *        small"), and GradientMagnitudeImageFilter throws for spacing == 0 (UseImageSpacing). @p minSpacing is the ITK
 *        tolerance for the caller (1e-8 for the recursive Gaussian; 0.0 for the central-difference gradient magnitude,
 *        which rejects only an exactly-zero spacing). A Z size of 1 is a 2D image, so spacing[2] is not checked there.
 *        The `!(abs(spacing) > minSpacing)` form also rejects a NaN spacing.
 */
inline Result<> ValidatePositiveSpacing(const FloatVec3& spacing, const SizeVec3& dims, int32 errorCode, double minSpacing)
{
  const uint32 effDim = (dims[2] > 1) ? 3u : 2u;
  for(uint32 a = 0; a < effDim; ++a)
  {
    if(!(std::abs(static_cast<double>(spacing[a])) > minSpacing))
    {
      return MakeErrorResult(
          errorCode, fmt::format("Image spacing for filtered axis {} is degenerate ({}); the Gaussian derivative divides by spacing, so it must be greater than {}.", a, spacing[a], minSpacing));
    }
  }
  return {};
}

/**
 * @brief Replicates ITK's `AnisotropicDiffusionImageFilter::InitializeIteration` non-fatal stability check: WARNS
 *        (never rejects) when the user's @p timeStep exceeds the CFL bound `minSpacing / 2^(ImageDimension+1)`,
 *        where `minSpacing` is the smallest spacing over the FILTERED axes (a Z size of 1 is a 2D image, so
 *        spacing[2] is excluded there, matching every other `effDim = (dims[2]>1) ? 3 : 2` guard in this file) and
 *        `ImageDimension` is that same effDim. Appends a @p warningCode @ref Warning to @p actions in place --
 *        @p actions is left VALID either way (a warning is never an error). Shared by the anisotropic-diffusion
 *        pair (GradientAnisotropicDiffusion and CurvatureAnisotropicDiffusion), which both pass the SAME shared
 *        warning code (@ref ImageProcessing::k_AnisotropicDiffusionUnstableTimeStep) for this condition.
 */
inline void AppendUnstableTimeStepWarning(Result<OutputActions>& actions, float64 timeStep, const FloatVec3& spacing, const SizeVec3& dims, int32 warningCode)
{
  const uint32 effDim = (dims[2] > 1) ? 3u : 2u;
  float64 minSpacing = static_cast<float64>(spacing[0]);
  for(uint32 a = 1; a < effDim; ++a)
  {
    minSpacing = std::min(minSpacing, static_cast<float64>(spacing[a]));
  }
  const float64 stableBound = minSpacing / std::pow(2.0, static_cast<double>(effDim) + 1.0);
  if(timeStep > stableBound)
  {
    actions.warnings().push_back(
        Warning{warningCode, fmt::format("Time Step ({}) exceeds the stable time step for this image ({}, = min filtered-axis spacing {} / 2^(ImageDimension+1) with ImageDimension={}); the "
                                         "anisotropic diffusion may be numerically unstable. This is a non-fatal warning (matching ITK), not an error.",
                                         timeStep, stableBound, minSpacing, effDim)});
  }
}

/**
 * @brief Validates a single Float64 @p value (labeled by @p label) against the input element type @p inputType,
 *        which may be INTEGER or FLOAT, dispatching through the @ref AllNumeric policy (all 10 scalar types).
 *
 * The object-morphology façade casts the value (Object Value / Background Value) to the input element type. For
 * an integer type that cast is UNDEFINED for a non-finite (@p nonFiniteCode) or out-of-range (@p outOfRangeCode)
 * value, and truncates a fractional value (@p truncatedWarnCode warning). For a float type only a non-finite
 * value is rejected (it would break the exact-equality object test). This is the all-scalar-type analogue of
 * @ref ValidateBackgroundInRange (which is integer-only). The three codes are supplied by the caller so each
 * filter keeps its own unique codes.
 */
inline Result<> ValidateScalarValueForType(DataType inputType, float64 value, const char* label, int32 nonFiniteCode, int32 outOfRangeCode, int32 truncatedWarnCode)
{
  return AllNumeric::dispatch(inputType, detail::ScalarValueRangeGuard{value, label, nonFiniteCode, outOfRangeCode, truncatedWarnCode});
}

/**
 * @brief Morphological watershed is a GLOBAL algorithm: ITK itself requires the whole image resident in RAM, and our
 *        engine floods on in-memory buffers (input element + uint32 markers + uint8 status + uint32 labels; the FAH
 *        front adds more, but it is data-dependent). Estimate that peak working set from @p numTuples and the input
 *        element type @p inputType and HARD-ERROR (with the caller's @p errorCode) at preflight when it would not fit
 *        in ~80% of the machine's currently-available RAM (total - used, from GetSystemMemoryInfo). This mirrors the
 *        ArrayCreationUtilities -264 precedent: watershed cannot stream larger-than-RAM data and stay ITK-exact, so it
 *        is rejected up front with a clear "needs ~X GB, ~Y GB available" message rather than thrashing the machine or
 *        dying with a std::bad_alloc mid-run. The estimate is a pure numeric computation -- it allocates nothing.
 *
 *        The ERROR takes precedence: it is only checked when the OS memory query succeeded (mem.totalGB > 0). When the
 *        working set instead FITS (or the query failed, so available RAM is UNKNOWN and the run is not blocked), a
 *        second concern remains: if the input is stored OUT-OF-CORE (@p inputIsOutOfCore), watershed will still load
 *        its whole working set into memory to run, silently overriding the user's out-of-core storage intent. That is
 *        surfaced as a non-fatal WARNING (with the caller's @p warningCode) so the override is not silent. The
 *        warning's working-set GB is computed the same way as the error's, so it is available even when the memory
 *        query failed. An IN-CORE input needs no warning (RAM is the expected backing -- no surprise).
 */
inline Result<OutputActions> ValidateWatershedFitsInMemory(DataType inputType, usize numTuples, bool inputIsOutOfCore, int32 errorCode, int32 warningCode)
{
  // Peak per-voxel bytes of the in-RAM working set that must coexist. The engine floods on four buffers -- input element
  // + uint32 marker (4) + uint8 status (1) + uint32 labels (4) -- but the FromMarkers execute path ALSO keeps two uint32
  // scratch stores (markerU32 + outU32 = 8) resident while the engine allocates those buffers, so count both:
  // sizeof(input) [engine inBuf] + 4+4 [markerU32+outU32] + 4 [engine marker] + 1 [engine status] + 4 [engine labels] =
  // sizeof(input) + 17. The composite path holds additional transient whole-volume scratch (regional-minima, labels,
  // HMinima), covered by the 80% budget headroom below plus the execute-time std::bad_alloc backstop in the ExecuteFns.
  const uint64 perVoxel = static_cast<uint64>(GetDataTypeSize(inputType)) + 17u /*markerU32(4)+outU32(4) + engine marker(4)+status(1)+labels(4)*/;
  const uint64 workingSetBytes = static_cast<uint64>(numTuples) * perVoxel;
  // totalGB/usedGB are GiB (bytes / 1024^3); keep the whole estimate + message on that one basis so the "needs" and
  // "available" figures a user sees share a basis (and available bytes are not under-counted by the ~7% GiB<->GB gap).
  constexpr double k_BytesPerGiB = 1024.0 * 1024.0 * 1024.0;
  const double workingSetGB = static_cast<double>(workingSetBytes) / k_BytesPerGiB;
  const nx::core::Memory::SystemMemoryInfo mem = nx::core::Memory::GetSystemMemoryInfo();
  // Only apply the hard ERROR when the OS memory query succeeded. If it failed (GetSystemMemoryInfo returns a
  // zero-initialized struct), treat available RAM as UNKNOWN and do NOT block -- rejecting on a "~0.0 GB available"
  // reading would hard-fail every run (even a 2x2x2) on a container where /proc/meminfo isn't readable or
  // GlobalMemoryStatusEx fails. The Task-1 engine's std::bad_alloc backstop still catches a genuine over-allocation at
  // execute time. Crucially, do NOT early-return here on query-failure: the OOC warning below is a pure numeric
  // estimate that needs no query, and must still be emitted.
  if(mem.totalGB > 0.0)
  {
    const double availableGB = std::max(0.0, mem.totalGB - mem.usedGB);
    const auto availableBytes = static_cast<uint64>(availableGB * k_BytesPerGiB);
    // Require the working set to fit with headroom (the FAH front + allocator overhead + other pipeline arrays).
    const uint64 budget = static_cast<uint64>(static_cast<double>(availableBytes) * 0.80);
    if(workingSetBytes > budget)
    {
      return MakeErrorResult<OutputActions>(errorCode, fmt::format("Morphological watershed is a global algorithm and must hold its working set (~{:.1f} GB for this "
                                                                   "volume) in memory; only ~{:.1f} GB is currently available. Reduce the volume, free memory, or run on "
                                                                   "a machine with more RAM. (Watershed cannot stream larger-than-RAM data and stay ITK-exact.)",
                                                                   workingSetGB, availableGB));
    }
  }
  // It fits (or the query failed): if the input is stored out-of-core, warn that watershed will still pull its whole
  // working set into memory to run, overriding the out-of-core storage intent. In-core input gets no warning.
  if(inputIsOutOfCore)
  {
    Result<OutputActions> result;
    result.warnings().push_back(Warning{warningCode, fmt::format("Morphological watershed is a global algorithm and cannot process out-of-core: it will load its working "
                                                                 "set (~{:.1f} GB) into memory to run, even though the input data is stored out-of-core. (The output labels "
                                                                 "are still written to out-of-core storage.)",
                                                                 workingSetGB)});
    return result;
  }
  return {};
}

// The sibling name a freshly-built collapsed geometry is created under while an in-place axis projection
// runs; a deferred rename/delete swap (see PreflightAxisProjection) moves it into the original geometry's
// place once executeImpl has finished reading the still-intact input. Mirrors the CropImageGeometry idiom.
// The leading dot follows the simplnx convention for a transient/hidden object that is not meant to
// survive the filter (it is deleted or renamed away by the deferred swap before the pipeline node returns).
inline const std::string k_ProjectionTempGeomName = ".projected_image_geometry";

/**
 * @brief Resolves the geometry the projected output array is created under. For a new-geometry run this
 *        is the user-named output geometry (created at the DataStructure root, matching the legacy
 *        name-only geometry-creation behavior); for an in-place run it is the temporary sibling geometry
 *        that is swapped into the original geometry's place by PreflightAxisProjection's deferred actions.
 *        Shared by the preflight façade and the filter's executeImpl so both agree on the path.
 */
inline DataPath ProjectionTargetGeomPath(const DataPath& imageGeomPath, bool performInPlace, const std::string& outputGeomName)
{
  return performInPlace ? imageGeomPath.replaceName(k_ProjectionTempGeomName) : DataPath({outputGeomName});
}

/**
 * @brief Resolves the full path of the projected output array. This is the SINGLE source of the output
 *        path derivation: PreflightAxisProjection (which allocates the array) and every projection
 *        filter's executeImpl (which writes into it) MUST agree byte-for-byte or the array goes missing,
 *        so both call this rather than re-deriving. The cell-data AttributeMatrix name is copied from the
 *        input geometry, which is still intact at both preflight and execute time (the in-place swap is
 *        deferred until after executeImpl).
 */
inline DataPath ProjectionOutputArrayPath(const DataStructure& dataStructure, const DataPath& imageGeomPath, bool performInPlace, const std::string& outputGeomName, const std::string& outputArrayName)
{
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const std::string cellDataName = imageGeom.getCellDataPath().getTargetName();
  return ProjectionTargetGeomPath(imageGeomPath, performInPlace, outputGeomName).createChildPath(cellDataName).createChildPath(outputArrayName);
}

/**
 * @brief Preflight for an image filter. Validates the input array's type against the compile-time
 *        ArrayTypeOptions bool-pack and against the geometry, then emits a CreateArrayAction for the
 *        output array. The output array inherits the input store's data format, so an OOC input
 *        produces an OOC output. @p chunkShapeHint optionally requests tuple-space chunks for the output store.
 */
template <class TypeSetPolicy = AllNumeric, template <class> class OutTypeMap = SameAsInput>
Result<OutputActions> PreflightImageFilter(const DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& imageGeomPath, const DataPath& outputArrayPath,
                                           bool requireScalar = false, std::optional<ShapeType> chunkShapeHint = {},
                                           DataStoreInitializationMode initializationMode = DataStoreInitializationMode::Default)
{
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);

  const DataType inputType = inputArray.getDataType();
  if(TypeSetPolicy::allowedTypes().count(inputType) == 0)
  {
    return MakeErrorResult<OutputActions>(k_UnsupportedDataType,
                                          fmt::format("Input array '{}' has DataType '{}', which is not supported by this filter.", inputArrayPath.toString(), DataTypeToString(inputType)));
  }

  // Opt-in single-component check (SH2). Left off by default so existing callers are unchanged; the grayscale
  // reconstruction filters pass requireScalar=true because their ITK operators are defined on scalar images.
  if(requireScalar && inputArray.getNumberOfComponents() != 1)
  {
    return MakeErrorResult<OutputActions>(
        k_NonScalarInput, fmt::format("This filter requires a single-component (scalar) input array, but '{}' has {} components.", inputArrayPath.toString(), inputArray.getNumberOfComponents()));
  }

  if(inputArray.getNumberOfTuples() != imageGeom.getNumberOfCells())
  {
    return MakeErrorResult<OutputActions>(k_GeometryMismatch, fmt::format("Input array '{}' has {} tuples but Image Geometry '{}' has {} cells.", inputArrayPath.toString(),
                                                                          inputArray.getNumberOfTuples(), imageGeomPath.toString(), imageGeom.getNumberOfCells()));
  }

  const DataType outputType = GetMappedOutputType<OutTypeMap>(inputType);

  OutputActions outputActions;
  outputActions.appendAction(std::make_unique<CreateArrayAction>(outputType, inputArray.getTupleShape(), inputArray.getComponentShape(), outputArrayPath, inputArray.getDataFormat(), "",
                                                                 std::move(chunkShapeHint), initializationMode));
  return {std::move(outputActions)};
}

template <class TypeSetPolicy = AllNumeric, template <class> class OutTypeMap = SameAsInput>
Result<OutputActions> PreflightFullOverwriteImageFilter(const DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& imageGeomPath, const DataPath& outputArrayPath,
                                                        bool requireScalar = false, std::optional<ShapeType> chunkShapeHint = {})
{
  return PreflightImageFilter<TypeSetPolicy, OutTypeMap>(dataStructure, inputArrayPath, imageGeomPath, outputArrayPath, requireScalar, std::move(chunkShapeHint),
                                                         DataStoreInitializationMode::DeferredZeroFill);
}

/**
 * @brief Preflight for an axis-projection image filter. Builds the output actions that collapse the
 *        selected axis (projDim: 0==X, 1==Y, 2==Z) to size 1, per the pinned output-shape rule:
 *          - the output ImageGeom dims are the input dims with dims[projDim] set to 1; origin & spacing
 *            are copied VERBATIM (never rescaled); the cell-data AttributeMatrix NAME is copied from the
 *            input geometry; the output array is allocated directly at the collapsed tuple shape
 *            (volume/dims[projDim] tuples) -- never full-size-then-resized.
 *          - performInPlace == false: a NEW geometry named @p outputGeomName holds the output array; the
 *            input geometry is left untouched.
 *          - performInPlace == true : the collapsed geometry is built under a temporary sibling name and,
 *            via DEFERRED actions, swapped into the original geometry's place AFTER executeImpl has read
 *            the (still-intact) input -- so the final output array lives at
 *            <origGeom>/<origCellData>/<outputArrayName> and the original geometry's dims become the
 *            collapsed dims. This mirrors the CropImageGeometry in-place idiom; a straight in-place resize
 *            is impossible because the collapsed output array cannot be inserted into the still-full cell
 *            AttributeMatrix (tuple-count mismatch), and resizing the AttributeMatrix before executeImpl
 *            would destroy the input the engine still has to read. Because that swap replaces the whole
 *            original geometry, any OTHER arrays in its cell data are discarded; a warning enumerates them.
 *
 * The output element type is the input type mapped through @p OutTypeMap (SameAsInput for type-preserving
 * reducers like Max; a fixed float map for Mean/StdDev/Sum). The execute façade MUST be instantiated with
 * the SAME OutTypeMap so the allocated type and the written type agree. The input array is validated to be
 * scalar (1 component), of a type allowed by @p TypeSetPolicy (ProjectionScalar's 4 types by default;
 * Binary passes AllNumeric), and tuple-consistent with the geometry. The output array inherits the input
 * store's data format, so an OOC input produces an OOC output.
 */
template <class TypeSetPolicy = ProjectionScalar, template <class> class OutTypeMap = SameAsInput>
Result<OutputActions> PreflightAxisProjection(const DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, usize projDim, bool performInPlace,
                                              const std::string& outputGeomName, const std::string& outputArrayName)
{
  if(projDim > 2)
  {
    return MakeErrorResult<OutputActions>(-8340, fmt::format("Projection Dimension must be 0 (X), 1 (Y), or 2 (Z), but got {}.", projDim));
  }

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);

  if(inputArray.getNumberOfComponents() != 1)
  {
    return MakeErrorResult<OutputActions>(
        -8341, fmt::format("Axis projection requires a single-component (scalar) input array, but '{}' has {} components.", inputArrayPath.toString(), inputArray.getNumberOfComponents()));
  }

  const DataType inputType = inputArray.getDataType();
  if(TypeSetPolicy::allowedTypes().count(inputType) == 0)
  {
    std::string supportedTypes;
    for(const DataType allowedType : TypeSetPolicy::allowedTypes())
    {
      if(!supportedTypes.empty())
      {
        supportedTypes += ", ";
      }
      supportedTypes += DataTypeToString(allowedType);
    }
    return MakeErrorResult<OutputActions>(k_UnsupportedDataType, fmt::format("Input array '{}' has DataType '{}', which this axis-projection filter does not support. Supported types: {}.",
                                                                             inputArrayPath.toString(), DataTypeToString(inputType), supportedTypes));
  }

  if(inputArray.getNumberOfTuples() != imageGeom.getNumberOfCells())
  {
    return MakeErrorResult<OutputActions>(k_GeometryMismatch, fmt::format("Input array '{}' has {} tuples but Image Geometry '{}' has {} cells.", inputArrayPath.toString(),
                                                                          inputArray.getNumberOfTuples(), imageGeomPath.toString(), imageGeom.getNumberOfCells()));
  }

  const AttributeMatrix* cellData = imageGeom.getCellData();
  if(cellData == nullptr)
  {
    return MakeErrorResult<OutputActions>(-8342, fmt::format("Image Geometry '{}' has no cell-data Attribute Matrix to project.", imageGeomPath.toString()));
  }
  const std::string cellDataName = imageGeom.getCellDataPath().getTargetName();

  // Collapsed geometry dims: the projected axis becomes size 1. Origin & spacing are copied verbatim
  // (NOT rescaled) per the pinned output-shape rule. dims are XYZ; the array tuple shape is ZYX.
  const SizeVec3 inputDims = imageGeom.getDimensions();
  CreateImageGeometryAction::DimensionType collapsedDims = {inputDims[0], inputDims[1], inputDims[2]};
  collapsedDims[projDim] = 1;
  const std::vector<usize> tupleShape = {collapsedDims[2], collapsedDims[1], collapsedDims[0]};

  const auto origin = imageGeom.getOrigin().toContainer<CreateImageGeometryAction::OriginType>();
  const auto spacing = imageGeom.getSpacing().toContainer<CreateImageGeometryAction::SpacingType>();

  // Output element type: the input type mapped through OutTypeMap (kept identical to the execute façade).
  const DataType outputDataType = GetMappedOutputType<OutTypeMap>(inputType);

  Result<OutputActions> result;

  const DataPath targetGeomPath = ProjectionTargetGeomPath(imageGeomPath, performInPlace, outputGeomName);

  // Allocate the collapsed geometry (and, below, its output array) DIRECTLY at the final shape.
  result.value().appendAction(std::make_unique<CreateImageGeometryAction>(targetGeomPath, collapsedDims, origin, spacing, cellDataName, imageGeom.getUnits()));

  const DataPath outputArrayPath = ProjectionOutputArrayPath(dataStructure, imageGeomPath, performInPlace, outputGeomName, outputArrayName);
  result.value().appendAction(std::make_unique<CreateArrayAction>(outputDataType, tupleShape, std::vector<usize>{1}, outputArrayPath, inputArray.getDataFormat(), "", std::nullopt,
                                                                  DataStoreInitializationMode::DeferredZeroFill));

  if(performInPlace)
  {
    // Deferred (post-executeImpl) swap: rename the original geometry away, delete it, then rename the
    // freshly-built collapsed geometry to the original name. Applied after executeImpl at execute time and
    // after the regular actions at preflight time, so downstream filters preflight against the final shape.
    const std::string originalName = imageGeomPath.getTargetName();
    const std::string renamedOriginalName = "." + originalName;
    const DataPath renamedOriginalPath = imageGeomPath.replaceName(renamedOriginalName);
    result.value().appendDeferredAction(std::make_unique<RenameDataAction>(imageGeomPath, renamedOriginalName));
    result.value().appendDeferredAction(std::make_unique<DeleteDataAction>(renamedOriginalPath));
    result.value().appendDeferredAction(std::make_unique<RenameDataAction>(targetGeomPath, originalName));

    // In-place discards the whole original geometry, so any OTHER cell-data arrays are lost. They cannot
    // survive the collapse (they have the original, non-collapsed tuple count), but say so rather than
    // dropping them silently. The selected input (consumed by the projection) is not listed.
    const std::string inputArrayName = inputArrayPath.getTargetName();
    std::string discarded;
    for(const auto& [childId, childObject] : *cellData)
    {
      const std::string childName = childObject->getName();
      if(childName == inputArrayName || childName == outputArrayName)
      {
        continue;
      }
      if(!discarded.empty())
      {
        discarded += ", ";
      }
      discarded += childName;
    }
    if(!discarded.empty())
    {
      result.warnings().push_back(Warning{-8343, fmt::format("Perform In-Place replaces Image Geometry '{}' with the collapsed projection, so the following other array(s) in cell data '{}' will be "
                                                             "discarded (they cannot be projected and do not fit the collapsed shape): {}. Disable 'Perform In-Place' to keep the original geometry.",
                                                             imageGeomPath.toString(), cellDataName, discarded)});
    }
  }

  return result;
}

/**
 * @brief The composite grayscale-morphology operation to apply, each built from the Dilate/Erode
 *        primitives (and, for the three difference forms, a single elementwise store subtraction):
 *          - Opening     = Dilate(Erode(in))            (removes bright specks smaller than the SE)
 *          - Closing     = Erode(Dilate(in))            (fills dark specks/holes smaller than the SE)
 *          - WhiteTopHat = in - Opening(in)             (the bright features the opening removed)
 *          - BlackTopHat = Closing(in) - in             (the dark features the closing filled)
 *          - Gradient    = Dilate(in) - Erode(in)       (the local intensity range == edge strength)
 */
enum class MorphCompositeOp
{
  Opening,
  Closing,
  WhiteTopHat,
  BlackTopHat,
  Gradient
};

/**
 * @brief The composite BINARY-morphology operation to apply, each built from the binary Dilate/Erode
 *        primitives (@ref ApplyBinaryMorphology):
 *          - Opening = binaryErode(boundaryToForeground=true) -> binaryDilate(boundaryToForeground=false)
 *                      (removes foreground specks/protrusions smaller than the structuring element). No padding.
 *          - Closing = binaryDilate(boundaryToForeground=false) -> binaryErode(boundaryToForeground=true)
 *                      (fills background holes/gaps smaller than the structuring element). Optional SafeBorder
 *                      pad/crop by the kernel radius.
 * These mirror the legacy ITK Binary Morphological Opening/Closing composites. On the strictly-{fg, bg} input
 * the binary morphology façade requires, ITK's restore-original-where-not-foreground post-pass is a proven
 * no-op, so it is intentionally omitted.
 */
enum class BinaryCompositeOp
{
  Opening,
  Closing
};

/**
 * @brief Specifies the maximum number of values in an elementwise staging chunk.
 * @tparam T Specifies the scalar value type.
 *
 * The 32 MiB target bounds each buffer while keeping bulk transfers large enough for out-of-core stores.
 */
template <class T>
inline constexpr usize k_ElementwiseChunkValues = std::max<usize>(usize{1}, (usize{32} << 20) / sizeof(T));

namespace detail
{
/**
 * @brief Applies an elementwise operation to a resident range in parallel.
 * @tparam T Specifies the scalar value type.
 * @tparam OperationT Specifies the operation for one index.
 * @param inputA First resident input buffer.
 * @param inputB Second resident input buffer. A unary operation can ignore this buffer.
 * @param output Resident output buffer.
 * @param count Number of values to process.
 * @param operation Applies the required arithmetic to one index.
 *
 * Worker threads use only raw resident pointers. Each index reads its operands before a possible aliased output write.
 */
template <class T, class OperationT>
void ApplyElementwiseResidentRange(const T* inputA, const T* inputB, T* output, usize count, OperationT operation)
{
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, count);
  parallelAlgorithm.execute([=](const Range& range) {
    const T* const inputAValuesPtr = inputA;
    const T* const inputBValuesPtr = inputB;
    T* const outputValuesPtr = output;
    const OperationT applyOperation = operation;
    for(usize index = range.min(); index < range.max(); ++index)
    {
      applyOperation(inputAValuesPtr, inputBValuesPtr, outputValuesPtr, index);
    }
  });
}

/**
 * @brief Subtracts two stores through bounded resident chunks.
 * @tparam T Specifies the scalar value type.
 * @param a First input store.
 * @param b Second input store.
 * @param out Output store.
 * @param shouldCancel Requests cancellation between chunks.
 * @param chunkValues Maximum number of values in each chunk.
 * @return An error from a bulk transfer, or a valid empty result.
 *
 * Serial bulk transfers isolate store access from parallel arithmetic. Both inputs are read before an aliased output write.
 * The arithmetic matches the resident path exactly.
 * Cancellation returns a valid result. The chunked path preserves completed output chunks, and a started resident operation runs to completion.
 */
template <class T>
Result<> SubtractStoresChunked(const AbstractDataStore<T>& a, const AbstractDataStore<T>& b, AbstractDataStore<T>& out, const std::atomic_bool& shouldCancel, usize chunkValues)
{
  const usize size = a.getSize();
  chunkValues = std::max<usize>(usize{1}, chunkValues);
  auto bufferA = std::make_unique_for_overwrite<T[]>(std::min(chunkValues, size));
  auto bufferB = std::make_unique_for_overwrite<T[]>(std::min(chunkValues, size));
  for(usize start = 0; start < size; start += chunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(chunkValues, size - start);
    if(Result<> result = a.copyIntoBuffer(start, nonstd::span<T>(bufferA.get(), count)); result.invalid())
    {
      return result;
    }
    if(Result<> result = b.copyIntoBuffer(start, nonstd::span<T>(bufferB.get(), count)); result.invalid())
    {
      return result;
    }
    ApplyElementwiseResidentRange(bufferA.get(), bufferB.get(), bufferA.get(), count, [](const T* aValues, const T* bValues, T* outValues, usize index) {
      const T aValue = aValues[index];
      const T bValue = bValues[index];
      outValues[index] = static_cast<T>(aValue - bValue);
    });
    if(Result<> result = out.copyFromBuffer(start, nonstd::span<const T>(bufferA.get(), count)); result.invalid())
    {
      return result;
    }
  }
  return {};
}

/**
 * @brief Inverts a binary store through bounded resident chunks.
 * @tparam T Specifies the scalar value type.
 * @param in Input store.
 * @param out Output store.
 * @param shouldCancel Requests cancellation between chunks.
 * @param chunkValues Maximum number of values in each chunk.
 * @return An error from a bulk transfer, or a valid empty result.
 *
 * Serial bulk transfers isolate store access from parallel arithmetic. Each input chunk is resident before an aliased output write.
 * The arithmetic matches the resident path exactly.
 * Cancellation returns a valid result. The chunked path preserves completed output chunks, and a started resident operation runs to completion.
 */
template <class T>
Result<> InvertBinaryStoreChunked(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const std::atomic_bool& shouldCancel, usize chunkValues)
{
  const usize size = in.getSize();
  chunkValues = std::max<usize>(usize{1}, chunkValues);
  auto buffer = std::make_unique_for_overwrite<T[]>(std::min(chunkValues, size));
  for(usize start = 0; start < size; start += chunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(chunkValues, size - start);
    if(Result<> result = in.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); result.invalid())
    {
      return result;
    }
    ApplyElementwiseResidentRange(buffer.get(), buffer.get(), buffer.get(), count, [](const T* inValues, const T*, T* outValues, usize index) {
      const T inValue = inValues[index];
      outValues[index] = (inValue != T{0}) ? T{0} : T{1};
    });
    if(Result<> result = out.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); result.invalid())
    {
      return result;
    }
  }
  return {};
}
} // namespace detail

/**
 * @brief Computes @c out[i] = a[i] - b[i] for equal-length scalar stores.
 * @tparam T Specifies the scalar value type.
 * @param a First input store.
 * @param b Second input store.
 * @param out Output store.
 * @param shouldCancel Requests cancellation before resident work or between chunks.
 * @param chunkValues Maximum number of values in each fallback chunk.
 * @return An error from a bulk transfer, or a valid empty result.
 *
 * Concrete in-memory stores use parallel resident spans. Other stores use bounded chunks with serial bulk transfers.
 * Both paths read each index before an aliased output write and use identical arithmetic.
 * Morphology callers order operands so valid neighborhoods produce nonnegative differences. A center-excluding kernel can have no in-bounds neighbor on a small image.
 * In that case, subtraction preserves ITK's extremum-seeded boundary result.
 * Cancellation returns a valid result. The chunked path preserves completed output chunks, and a started resident operation runs to completion.
 * @pre The three stores have equal lengths.
 */
template <class T>
Result<> SubtractStores(const AbstractDataStore<T>& a, const AbstractDataStore<T>& b, AbstractDataStore<T>& out, const std::atomic_bool& shouldCancel, usize chunkValues = k_ElementwiseChunkValues<T>)
{
  const auto* residentAPtr = dynamic_cast<const DataStore<T>*>(&a);
  const auto* residentBPtr = dynamic_cast<const DataStore<T>*>(&b);
  auto* residentOutPtr = dynamic_cast<DataStore<T>*>(&out);
  if(residentAPtr != nullptr && residentBPtr != nullptr && residentOutPtr != nullptr)
  {
    if(shouldCancel)
    {
      return {};
    }
    const auto aValues = residentAPtr->createSpan();
    const auto bValues = residentBPtr->createSpan();
    auto outValues = residentOutPtr->createSpan();
    detail::ApplyElementwiseResidentRange(aValues.data(), bValues.data(), outValues.data(), aValues.size(), [](const T* aData, const T* bData, T* outData, usize index) {
      const T aValue = aData[index];
      const T bValue = bData[index];
      outData[index] = static_cast<T>(aValue - bValue);
    });
    return {};
  }
  return detail::SubtractStoresChunked(a, b, out, shouldCancel, chunkValues);
}

/**
 * @brief Maps each nonzero input value to zero and each zero input value to one.
 * @tparam T Specifies the scalar value type.
 * @param in Input store.
 * @param out Output store.
 * @param shouldCancel Requests cancellation before resident work or between chunks.
 * @param chunkValues Maximum number of values in each fallback chunk.
 * @return An error from a bulk transfer, or a valid empty result.
 *
 * Concrete in-memory stores use parallel resident spans. Other stores use bounded chunks with serial bulk transfers.
 * Both paths read each index before an aliased output write and use identical arithmetic.
 * Signed Danielsson uses this operation to build the input for its second distance transform.
 * Cancellation returns a valid result. The chunked path preserves completed output chunks, and a started resident operation runs to completion.
 * @pre The two stores have equal lengths.
 */
template <class T>
Result<> InvertBinaryStore(const AbstractDataStore<T>& in, AbstractDataStore<T>& out, const std::atomic_bool& shouldCancel, usize chunkValues = k_ElementwiseChunkValues<T>)
{
  const auto* residentInPtr = dynamic_cast<const DataStore<T>*>(&in);
  auto* residentOutPtr = dynamic_cast<DataStore<T>*>(&out);
  if(residentInPtr != nullptr && residentOutPtr != nullptr)
  {
    if(shouldCancel)
    {
      return {};
    }
    const auto inValues = residentInPtr->createSpan();
    auto outValues = residentOutPtr->createSpan();
    detail::ApplyElementwiseResidentRange(inValues.data(), inValues.data(), outValues.data(), inValues.size(), [](const T* inData, const T*, T* outData, usize index) {
      const T inValue = inData[index];
      outData[index] = (inValue != T{0}) ? T{0} : T{1};
    });
    return {};
  }
  return detail::InvertBinaryStoreChunked(in, out, shouldCancel, chunkValues);
}

/**
 * @brief In-place negates every value of a scalar store (out[i] = -out[i]) using bounded chunks.
 */
template <class T>
Result<> NegateStore(AbstractDataStore<T>& store, const std::atomic_bool& shouldCancel)
{
  const usize size = store.getSize();
  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<T[]>(std::min(k_ChunkValues, size));
  for(usize start = 0; start < size; start += k_ChunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkValues, size - start);
    if(Result<> r = store.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
    {
      return r;
    }
    for(usize i = 0; i < count; ++i)
    {
      buffer[i] = static_cast<T>(-buffer[i]);
    }
    if(Result<> r = store.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

namespace detail
{
template <template <class> class OutTypeMap, class OperationT>
struct PointwiseExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  const OperationT& operation;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    using U = OutTypeMap<T>;
    const auto& inputStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outputStore = dataStructure.getDataRefAs<DataArray<U>>(outputArrayPath).getDataStoreRef();
    auto mapOp = operation.template makeMapOp<T, U>();
    return ApplyPointwise<T, U>(inputStore, outputStore, mapOp, shouldCancel, messageHandler);
  }
};

template <template <class> class OutTypeMap, class OperationT>
struct TwoPassExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  const OperationT& operation;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    using U = OutTypeMap<T>;
    const auto& inputStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outputStore = dataStructure.getDataRefAs<DataArray<U>>(outputArrayPath).getDataStoreRef();

    Result<ArrayStatistics<T>> statsResult = [&]() {
      if constexpr(requires { operation.template computeStatistics<T>(inputStore, shouldCancel); })
      {
        return operation.template computeStatistics<T>(inputStore, shouldCancel);
      }
      else
      {
        return ComputeArrayStatistics<T>(inputStore, shouldCancel);
      }
    }();
    if(statsResult.invalid())
    {
      return {nonstd::make_unexpected(std::move(statsResult.errors()))};
    }
    if(shouldCancel)
    {
      return {};
    }
    const ArrayStatistics<T>& stats = statsResult.value();

    if(Result<> validation = operation.template validate<T>(stats); validation.invalid())
    {
      return validation;
    }
    auto mapOp = operation.template makeMapOp<T, U>(stats);
    return ApplyPointwise<T, U>(inputStore, outputStore, mapOp, shouldCancel, messageHandler);
  }
};

struct MaskExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& maskArrayPath;
  const DataPath& outputArrayPath;
  usize numComponents;
  float64 outsideValue;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inputStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outputStore = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath).getDataStoreRef();
    const auto& maskArray = dataStructure.getDataRefAs<IDataArray>(maskArrayPath);
    return ApplyMask<T>(inputStore, outputStore, maskArray, numComponents, static_cast<T>(outsideValue), shouldCancel, messageHandler);
  }
};

template <class OperationT>
struct BoxNeighborhoodExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  std::array<usize, 3> radius;
  const OperationT& operation;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inputStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outputStore = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath).getDataStoreRef();
    auto reduceFn = operation.template makeReduceOp<T>();
    return ApplyBoxNeighborhood<T>(inputStore, outputStore, dims, radius, reduceFn, shouldCancel, messageHandler);
  }
};

template <class PredicateFactoryT>
struct ContourExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  const std::vector<SEOffset>& neighborOffsets;
  const PredicateFactoryT& predicateFactory;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    // Output type == input type (SameAsInput). The factory casts the Float64 fg/bg parameters to the integer
    // element type here; the caller's preflight guard has already rejected non-finite/out-of-range values, so
    // the narrowing conversion is well-defined.
    const auto& inStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outStore = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath).getDataStoreRef();
    const auto predicate = predicateFactory.template make<T>();
    return ApplyContour<T>(inStore, outStore, dims, neighborOffsets, predicate, shouldCancel, messageHandler);
  }
};

struct MorphologyExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  const StructuringElement& se;
  MorphOp op;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    // The in/out IDataArrays are needed by ApplyMorphology's DispatchAlgorithm to decide the storage path;
    // the typed stores are the buffers the engine reads/writes. Output type == input type (SameAsInput).
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    return ApplyMorphology<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, se, op, inArray, outArray, shouldCancel, messageHandler);
  }
};

struct ObjectMorphologyExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  const StructuringElement& se;
  const std::vector<SEOffset>& boxOffsets;
  ObjectMorphOp op;
  float64 objectValue;
  float64 backgroundValue;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    // The in/out IDataArrays are needed by ApplyObjectMorphology's DispatchAlgorithm to decide the storage
    // path; the typed stores are the buffers the engine reads/writes. Output type == input type (SameAsInput).
    // The Float64 object/background values are cast to the element type here; the caller's preflight guard has
    // already rejected non-finite/out-of-range values, so the narrowing conversion is well-defined.
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    return ApplyObjectMorphology<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, se, boxOffsets, op, static_cast<T>(objectValue), static_cast<T>(backgroundValue), inArray, outArray,
                                    shouldCancel, messageHandler);
  }
};

struct AdaptiveHistogramEqualizationExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  std::array<usize, 3> radius;
  float32 alpha;
  float32 beta;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    // Output type == input type (SameAsInput). The engine is single-implementation (no DispatchAlgorithm), so
    // it needs only the typed stores.
    const auto& inputStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outputStore = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath).getDataStoreRef();
    return ApplyAdaptiveHistogramEqualization<T>(inputStore, outputStore, dims, radius, alpha, beta, shouldCancel, messageHandler);
  }
};

/**
 * @brief Publishes a candidate index when it is smaller than the current index.
 * @param smallest Atomic index that receives the minimum published value.
 * @param candidate Index to compare with the current minimum.
 *
 * The compare-and-exchange loop makes the result independent of the order in which workers publish candidates.
 */
inline void PublishSmallerIndex(std::atomic<usize>& smallest, usize candidate) noexcept
{
  usize current = smallest.load(std::memory_order_relaxed);
  while(candidate < current && !smallest.compare_exchange_weak(current, candidate, std::memory_order_relaxed))
  {
  }
}

/**
 * @brief Finds the first value that does not match either binary value.
 * @tparam T Specifies the resident value type.
 * @param values Caller-owned resident values to compare.
 * @param fg Foreground value.
 * @param bg Background value.
 * @return Smallest invalid index, or no value when the span is empty or all values are binary.
 *
 * The parallel comparison is exact because an atomic minimum combines the first violation from each range.
 * A range that starts at or after the known minimum cannot produce a smaller index, so the worker safely skips it.
 * Worker threads read only the caller-owned resident span, so they do not access a data store.
 * Cancellation is checked by the caller between resident chunks, so a chunk that contains a violation always reports it.
 */
template <class T>
std::optional<usize> FindFirstNonBinaryValue(nonstd::span<const T> values, T fg, T bg)
{
  constexpr usize k_ScanBlockValues = 4096;
  std::atomic<usize> firstInvalid{values.size()};
  // The block OR-reduction has no data-dependent exit, so compilers can vectorize it.
  // The first-violation search runs only in the first block of each range that contains a violation.
  // The per-block check bounds unused work after another range publishes a smaller index.
  const T* const data = values.data();
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(0, values.size());
  parallelAlgorithm.execute([&](const Range& range) {
    const usize known = firstInvalid.load(std::memory_order_relaxed);
    if(range.min() >= known)
    {
      return;
    }
    // Local copies keep both reference values in registers; a by-reference capture is reloaded inside the loop.
    const T foreground = fg;
    const T background = bg;
    for(usize blockBegin = range.min(); blockBegin < range.max(); blockBegin += k_ScanBlockValues)
    {
      if(blockBegin >= firstInvalid.load(std::memory_order_relaxed))
      {
        return;
      }
      const usize blockEnd = std::min(range.max(), blockBegin + k_ScanBlockValues);
      uint32 invalidMask = 0;
      for(usize i = blockBegin; i < blockEnd; ++i)
      {
        invalidMask |= static_cast<uint32>(data[i] != foreground) & static_cast<uint32>(data[i] != background);
      }
      if(invalidMask != 0)
      {
        for(usize i = blockBegin; i < blockEnd; ++i)
        {
          if(data[i] != foreground && data[i] != background)
          {
            PublishSmallerIndex(firstInvalid, i);
            return;
          }
        }
      }
    }
  });

  const usize invalid = firstInvalid.load(std::memory_order_relaxed);
  if(invalid == values.size())
  {
    return std::nullopt;
  }
  return invalid;
}

/**
 * @brief Specifies the default number of resident values for a binary input scan.
 * @tparam T Specifies the input value type that determines the 64 MiB value count.
 */
template <class T>
inline constexpr usize k_BinaryScanChunkValues = std::max<usize>(usize{1}, (usize{64} << 20) / sizeof(T));

/**
 * @brief Validates that a binary morphology input contains only the foreground and background values.
 * @tparam T Specifies the input value type.
 * @param inStore Store that contains the original input values.
 * @param fg Foreground value that the binary morphology engine accepts.
 * @param bg Background value that the binary morphology engine accepts.
 * @param inputArrayPath Path included in a non-binary input error.
 * @param shouldCancel Shared cancellation flag.
 * @param chunkValues Requested maximum number of values in each resident chunk. Values less than one select one value.
 * @return Error for a store-read failure or the first non-binary value. A binary or cancelled scan returns a valid result.
 *
 * The engine emits a strict {foreground, background} image. ITK preserves each original non-foreground value, so equivalent behavior requires a strict binary input.
 * Store reads are serial and bounded. The function compares each resident chunk in parallel.
 * Standalone binary Dilate and Erode, binary Opening and Closing composites, and Binary Opening By Reconstruction scan the original input exactly once.
 * The callers do not scan engine-created intermediate stores because those stores contain only the foreground and background values.
 */
template <class T>
Result<> ScanBinaryInput(const AbstractDataStore<T>& inStore, T fg, T bg, const DataPath& inputArrayPath, const std::atomic_bool& shouldCancel, usize chunkValues = k_BinaryScanChunkValues<T>)
{
  const usize size = inStore.getSize();
  chunkValues = std::max<usize>(usize{1}, chunkValues);
  auto scanBuffer = std::make_unique<T[]>(std::min(chunkValues, size));
  for(usize start = 0; start < size; start += chunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(chunkValues, size - start);
    if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(scanBuffer.get(), count)); r.invalid())
    {
      return r;
    }
    if(const std::optional<usize> invalid = FindFirstNonBinaryValue<T>(nonstd::span<const T>(scanBuffer.get(), count), fg, bg); invalid.has_value())
    {
      return MakeErrorResult(k_NonBinaryInput,
                             fmt::format("Binary morphology requires a binary image containing only the foreground ({}) or background ({}) value, but input array '{}' contains the value {} at "
                                         "index {}. Threshold or relabel the input first.",
                                         static_cast<int64>(fg), static_cast<int64>(bg), inputArrayPath.toString(), static_cast<int64>(scanBuffer[*invalid]), start + *invalid));
    }
  }
  return {};
}

struct BinaryMorphologyExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  const StructuringElement& se;
  MorphOp op;
  float64 foreground;
  float64 background;
  bool boundaryToForeground;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    // The in/out IDataArrays are needed by ApplyBinaryMorphology's DispatchAlgorithm to decide the storage
    // path; the typed stores are the buffers the engine reads/writes. Output type == input type (SameAsInput).
    // The Float64 foreground/background are cast to the (integer) element type here; the caller's preflight
    // guard has already rejected non-finite/out-of-range values, so the narrowing conversion is well-defined.
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    const T foregroundT = static_cast<T>(foreground);
    const T backgroundT = static_cast<T>(background);

    // Enforce the strictly-{fg, bg} binary-input contract before running the engine (see ScanBinaryInput).
    if(Result<> r = ScanBinaryInput<T>(inStore, foregroundT, backgroundT, inputArrayPath, shouldCancel); r.invalid())
    {
      return r;
    }

    return ApplyBinaryMorphology<T>(inStore, outArray.getDataStoreRef(), dims, se, op, foregroundT, backgroundT, boundaryToForeground, inArray, outArray, shouldCancel, messageHandler);
  }
};

/**
 * @brief Per-axis non-negative structuring-element radius (a negative radius means an empty axis; clamp to 0).
 *        This is exactly the pad/crop amount ITK's SafeBorder open/close uses (@c GetKernel().GetRadius()).
 */
inline std::array<usize, 3> SERadius(const StructuringElement& se)
{
  return {se.radius[0] > 0 ? static_cast<usize>(se.radius[0]) : 0, se.radius[1] > 0 ? static_cast<usize>(se.radius[1]) : 0, se.radius[2] > 0 ? static_cast<usize>(se.radius[2]) : 0};
}

/**
 * @brief The SafeBorder pad/crop amount per axis: @ref SERadius, but with the Z-axis pad forced to 0 when the image
 *        is 2D (@p dims[2] == 1). ITK's ONLY dimensional collapse is Z==1 => 2D (ITKArrayHelper.hpp: the
 *        tDims.getZ()==1 branch runs an itk::Image<T,2> with a 2D kernel), so its SafeBorder ConstantPad pads X and
 *        Y but never Z on a 2D image. Padding Z here would give it real depth that the structuring element's
 *        out-of-plane (dz!=0) offsets then read, diverging from ITK on 2D Ball/Cross SEs (the composite Safe Border
 *        defect this reproduces).
 *
 *        Only Z is clamped, mirroring ITK exactly. Do NOT generalize this to "any axis whose extent is 1": ITK does
 *        not collapse X or Y, so on an image ITK keeps 3D (dims[2] > 1) a size-1 X or Y axis is still padded by its
 *        SE radius -- clamping it would introduce a new divergence. The reported bug is only the Z axis of a 2D
 *        image, and that is all this clamps.
 */
inline std::array<usize, 3> SafeBorderPadRadius(const StructuringElement& se, const SizeVec3& dims)
{
  const std::array<usize, 3> r = SERadius(se);
  return {r[0], r[1], dims[2] == 1 ? usize{0} : r[2]};
}

/**
 * @brief Copies @p in (@p dims) into the interior of @p padded (dims + 2*@p r per axis), filling the
 *        @p r-thick border on every side with @p padValue. Streams one padded Z-plane at a time (bounded ==
 *        two planes), so it is identical in-core and out-of-core. This reproduces ITK's SafeBorder
 *        ConstantPadImageFilter step (pad by the kernel radius with the op's extremum) ahead of the two
 *        morphology passes.
 */
template <class T>
Result<> PadIntoStore(const AbstractDataStore<T>& in, AbstractDataStore<T>& padded, const SizeVec3& dims, const std::array<usize, 3>& r, T padValue, const std::atomic_bool& shouldCancel)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize pdimX = dimX + 2 * r[0];
  const usize pdimY = dimY + 2 * r[1];
  const usize pdimZ = dimZ + 2 * r[2];
  const usize inSlice = dimX * dimY;
  const usize pSlice = pdimX * pdimY;

  auto paddedPlane = std::make_unique<T[]>(pSlice);
  auto inPlane = std::make_unique<T[]>(inSlice);

  for(usize pz = 0; pz < pdimZ; ++pz)
  {
    if(shouldCancel)
    {
      return {};
    }
    // Border-fill the whole padded plane; interior rows are overwritten below when this plane maps to input.
    std::fill(paddedPlane.get(), paddedPlane.get() + pSlice, padValue);
    if(pz >= r[2] && pz < r[2] + dimZ)
    {
      const usize iz = pz - r[2];
      if(Result<> res = in.copyIntoBuffer(iz * inSlice, nonstd::span<T>(inPlane.get(), inSlice)); res.invalid())
      {
        return res;
      }
      for(usize iy = 0; iy < dimY; ++iy)
      {
        const usize py = iy + r[1];
        std::copy(inPlane.get() + iy * dimX, inPlane.get() + (iy + 1) * dimX, paddedPlane.get() + py * pdimX + r[0]);
      }
    }
    if(Result<> res = padded.copyFromBuffer(pz * pSlice, nonstd::span<const T>(paddedPlane.get(), pSlice)); res.invalid())
    {
      return res;
    }
  }
  return {};
}

/**
 * @brief Copies the original-extent interior (offset @p r on every side) of @p padded (dims + 2*@p r) back
 *        into @p out (@p dims). Streams one Z-plane at a time (bounded), identical in-core and OOC. This is
 *        ITK's SafeBorder CropImageFilter step (crop by the kernel radius) after the two morphology passes.
 */
template <class T>
Result<> CropFromStore(const AbstractDataStore<T>& padded, AbstractDataStore<T>& out, const SizeVec3& dims, const std::array<usize, 3>& r, const std::atomic_bool& shouldCancel)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize pdimX = dimX + 2 * r[0];
  const usize pdimY = dimY + 2 * r[1];
  const usize outSlice = dimX * dimY;
  const usize pSlice = pdimX * pdimY;

  auto paddedPlane = std::make_unique<T[]>(pSlice);
  auto outPlane = std::make_unique<T[]>(outSlice);

  for(usize z = 0; z < dimZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize pz = z + r[2];
    if(Result<> res = padded.copyIntoBuffer(pz * pSlice, nonstd::span<T>(paddedPlane.get(), pSlice)); res.invalid())
    {
      return res;
    }
    for(usize y = 0; y < dimY; ++y)
    {
      const usize py = y + r[1];
      std::copy(paddedPlane.get() + py * pdimX + r[0], paddedPlane.get() + py * pdimX + r[0] + dimX, outPlane.get() + y * dimX);
    }
    if(Result<> res = out.copyFromBuffer(z * outSlice, nonstd::span<const T>(outPlane.get(), outSlice)); res.invalid())
    {
      return res;
    }
  }
  return {};
}

/**
 * @brief Selects which operand appears first in @ref CropSubtractFromStore.
 */
enum class CropSubtractOrder
{
  OriginalMinusCropped,
  CroppedMinusOriginal
};

/// @brief Fuses the SafeBorder top-hat crop and subtraction.
/// @details Reads both source z-planes before writing the output z-plane.
/// Therefore, @p out may alias @p original safely.
/// Subtraction uses scalar @p T arithmetic and casts back to @p T.
/// @pre Every store has component shape {1}.
/// @pre Original and output tuple shapes are {dims[2], dims[1], dims[0]}.
/// @pre Padded has that tuple shape plus twice @p r on the corresponding axes.
/// @pre @p padded does not alias @p original or @p out.
template <class T>
Result<> CropSubtractFromStore(const AbstractDataStore<T>& padded, const AbstractDataStore<T>& original, AbstractDataStore<T>& out, const SizeVec3& dims, const std::array<usize, 3>& r,
                               CropSubtractOrder order, const std::atomic_bool& shouldCancel)
{
  if(shouldCancel)
  {
    return {};
  }

  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize pdimX = dimX + 2 * r[0];
  const usize pdimY = dimY + 2 * r[1];
  const usize outSlice = dimX * dimY;
  const usize pSlice = pdimX * pdimY;

  auto paddedPlane = std::make_unique<T[]>(pSlice);
  auto originalPlane = std::make_unique<T[]>(outSlice);

  for(usize z = 0; z < dimZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize pz = z + r[2];
    if(Result<> res = padded.copyIntoBuffer(pz * pSlice, nonstd::span<T>(paddedPlane.get(), pSlice)); res.invalid())
    {
      return res;
    }
    if(Result<> res = original.copyIntoBuffer(z * outSlice, nonstd::span<T>(originalPlane.get(), outSlice)); res.invalid())
    {
      return res;
    }
    for(usize y = 0; y < dimY; ++y)
    {
      const usize py = y + r[1];
      const T* croppedRow = paddedPlane.get() + py * pdimX + r[0];
      T* outputRow = originalPlane.get() + y * dimX;
      if(order == CropSubtractOrder::OriginalMinusCropped)
      {
        for(usize x = 0; x < dimX; ++x)
        {
          outputRow[x] = static_cast<T>(outputRow[x] - croppedRow[x]);
        }
      }
      else
      {
        for(usize x = 0; x < dimX; ++x)
        {
          outputRow[x] = static_cast<T>(croppedRow[x] - outputRow[x]);
        }
      }
    }
    if(Result<> res = out.copyFromBuffer(z * outSlice, nonstd::span<const T>(originalPlane.get(), outSlice)); res.invalid())
    {
      return res;
    }
  }
  return {};
}

struct MorphologyCompositeExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  const StructuringElement& se;
  MorphCompositeOp op;
  bool safeBorder;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<T>& outStore = outArray.getDataStoreRef();
    const bool hasActiveOocEndpoint = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || outStore.getStoreType() == IDataStore::StoreType::OutOfCore;
    const std::string scratchFormat = inStore.getStoreType() == IDataStore::StoreType::OutOfCore  ? inArray.getDataFormat() :
                                      outStore.getStoreType() == IDataStore::StoreType::OutOfCore ? outArray.getDataFormat() :
                                                                                                    inArray.getDataFormat();

    const auto tryMorphologyPipeline = [&](MorphOp firstOp, MorphOp secondOp, detail::MorphologyCompositeOutputMode outputMode) -> std::optional<Result<>> {
      if(!hasActiveOocEndpoint)
      {
        return std::nullopt;
      }
      auto requiredBytesResult = CalculateMorphologyCompositePipelineWorkingMemoryBytes<T>(dims, se, safeBorder);
      if(requiredBytesResult.invalid())
      {
        return std::nullopt;
      }
      auto reservation = ReserveWorkingMemory(requiredBytesResult.value(), requiredBytesResult.value());
      if(reservation.sizeBytes() != requiredBytesResult.value())
      {
        return std::nullopt;
      }
      return ApplyMorphologyCompositePipeline<T>(inStore, outStore, dims, se, firstOp, secondOp, safeBorder, outputMode, shouldCancel, static_cast<usize>(reservation.sizeBytes()));
    };

    // Computes the grayscale opening (firstOp==Erode) or closing (firstOp==Dilate). Ordinarily the cropped
    // original extent is written to dst. A cropSubtractOrder instead fuses that final crop with the top-hat
    // subtraction into dst. OOC detection uses the real in/out arrays; every scratch inherits their storage
    // mode, so all passes take the same in-core Direct or OOC Scanline path and stay bounded-memory.
    auto openClose = [&](AbstractDataStore<T>& dst, MorphOp firstOp, MorphOp secondOp, std::optional<CropSubtractOrder> cropSubtractOrder = std::nullopt) -> Result<> {
      if(!safeBorder)
      {
        // SafeBorder == false (== ITK with SafeBorder off): plain two-pass composition with skip-OOB
        // (extremum-fill) at the true image edge. ONE same-extent scratch: firstOp(in) -> scratch;
        // secondOp(scratch) -> dst. Each pass reads a store distinct from the one it writes.
        auto scratch = DataStoreUtilities::CreateDataStoreWithFormat<T>(scratchFormat, inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
        if(Result<> r = ApplyMorphology<T>(inStore, *scratch, dims, se, firstOp, inArray, outArray, shouldCancel, messageHandler); r.invalid())
        {
          return r;
        }
        return ApplyMorphology<T>(*scratch, dst, dims, se, secondOp, inArray, outArray, shouldCancel, messageHandler);
      }

      // SafeBorder == true (ITK default): pad the volume by the per-axis kernel radius, fill the border with
      // the op's boundary extremum (Erode-first -> max(); Dilate-first -> lowest(); ITK's pad constants), run
      // both passes on the padded volume, then crop the original extent back out. Because we pad by exactly
      // the radius, the second pass never reaches the padded edge over the cropped region, so its own
      // boundary handling is irrelevant to the result -- reproducing ITK's Pad/erode/dilate/Crop exactly.
      const std::array<usize, 3> r = SafeBorderPadRadius(se, dims);
      const SizeVec3 pdims = {dims[0] + 2 * r[0], dims[1] + 2 * r[1], dims[2] + 2 * r[2]};
      const std::vector<usize> paddedShape = {pdims[2], pdims[1], pdims[0]}; // ZYX tuple shape
      const T padValue = (firstOp == MorphOp::Erode) ? std::numeric_limits<T>::max() : std::numeric_limits<T>::lowest();

      auto paddedIn = DataStoreUtilities::CreateDataStoreWithFormat<T>(scratchFormat, paddedShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
      auto paddedTmp = DataStoreUtilities::CreateDataStoreWithFormat<T>(scratchFormat, paddedShape, std::vector<usize>{1}, IDataAction::Mode::Execute);

      if(Result<> r2 = PadIntoStore<T>(inStore, *paddedIn, dims, r, padValue, shouldCancel); r2.invalid())
      {
        return r2;
      }
      if(Result<> r2 = ApplyMorphology<T>(*paddedIn, *paddedTmp, pdims, se, firstOp, inArray, outArray, shouldCancel, messageHandler); r2.invalid())
      {
        return r2;
      }
      if(Result<> r2 = ApplyMorphology<T>(*paddedTmp, *paddedIn, pdims, se, secondOp, inArray, outArray, shouldCancel, messageHandler); r2.invalid())
      {
        return r2;
      }
      if(cropSubtractOrder.has_value())
      {
        return CropSubtractFromStore<T>(*paddedIn, inStore, dst, dims, r, *cropSubtractOrder, shouldCancel);
      }
      return CropFromStore<T>(*paddedIn, dst, dims, r, shouldCancel);
    };

    switch(op)
    {
    case MorphCompositeOp::Opening: {
      if(auto result = tryMorphologyPipeline(MorphOp::Erode, MorphOp::Dilate, detail::MorphologyCompositeOutputMode::Morphology); result.has_value())
      {
        return *result;
      }
      return openClose(outStore, MorphOp::Erode, MorphOp::Dilate);
    }
    case MorphCompositeOp::Closing: {
      if(auto result = tryMorphologyPipeline(MorphOp::Dilate, MorphOp::Erode, detail::MorphologyCompositeOutputMode::Morphology); result.has_value())
      {
        return *result;
      }
      return openClose(outStore, MorphOp::Dilate, MorphOp::Erode);
    }
    case MorphCompositeOp::WhiteTopHat: {
      // WhiteTopHat = in - Opening(in). SafeBorder fuses the padded crop and subtraction; the unpadded path
      // retains the original open-into-out followed by alias-safe subtraction sequence.
      if(auto result = tryMorphologyPipeline(MorphOp::Erode, MorphOp::Dilate, detail::MorphologyCompositeOutputMode::OriginalMinusMorphology); result.has_value())
      {
        return *result;
      }
      if(safeBorder)
      {
        return openClose(outStore, MorphOp::Erode, MorphOp::Dilate, CropSubtractOrder::OriginalMinusCropped);
      }
      if(Result<> r = openClose(outStore, MorphOp::Erode, MorphOp::Dilate); r.invalid())
      {
        return r;
      }
      return SubtractStores<T>(inStore, outStore, outStore, shouldCancel);
    }
    case MorphCompositeOp::BlackTopHat: {
      // BlackTopHat = Closing(in) - in. SafeBorder fuses the padded crop and subtraction; the unpadded path
      // retains the original close-into-out followed by alias-safe subtraction sequence.
      if(auto result = tryMorphologyPipeline(MorphOp::Dilate, MorphOp::Erode, detail::MorphologyCompositeOutputMode::MorphologyMinusOriginal); result.has_value())
      {
        return *result;
      }
      if(safeBorder)
      {
        return openClose(outStore, MorphOp::Dilate, MorphOp::Erode, CropSubtractOrder::CroppedMinusOriginal);
      }
      if(Result<> r = openClose(outStore, MorphOp::Dilate, MorphOp::Erode); r.invalid())
      {
        return r;
      }
      return SubtractStores<T>(outStore, inStore, outStore, shouldCancel);
    }
    case MorphCompositeOp::Gradient: {
      // Gradient == max(window) - min(window), fused into a SINGLE pass that tracks BOTH window extremes at
      // once (in-core moving-histogram Direct or OOC Z-slab Scanline), replacing the former dilate-pass +
      // erode-pass + SubtractStores three-pass sequence. Byte-identical: for any window max - min equals
      // dilate - erode (the same two extremes of the same neighborhood, the same integer subtraction),
      // including the empty-neighborhood corner (a center-excluding SE with no in-bounds neighbor ->
      // static_cast<T>(lowest() - max())) and the degenerate empty SE (radius-0 Annulus -> 0 == in - in). No
      // SafeBorder (ITK's Morphological Gradient has none), so @c safeBorder is ignored -- and no scratch store
      // is needed (the fused pass writes the range directly into out).
      return ApplyMorphologyGradient<T>(inStore, outStore, dims, se, inArray, outArray, shouldCancel, messageHandler);
    }
    }
    return {};
  }
};

struct BinaryMorphologyCompositeExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  const StructuringElement& se;
  BinaryCompositeOp op;
  float64 foreground;
  float64 background; // used only by Opening; Closing derives its internal background
  bool safeBorder;    // used only by Closing (ITK binary Opening has no SafeBorder)
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<T>& outStore = outArray.getDataStoreRef();
    const bool hasActiveOocEndpoint = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || outStore.getStoreType() == IDataStore::StoreType::OutOfCore;

    const T fgT = static_cast<T>(foreground);

    // Resolve the background value that both passes write for non-foreground voxels:
    //  - Opening is anti-extensive and keeps the user's background, so it uses the user-supplied value (already
    //    range-validated at preflight, so the cast is well-defined).
    //  - Closing is extensive (it never adds background), so ITK exposes no background for it and DERIVES one.
    //    This reproduces itk::BinaryMorphologicalClosingImageFilter exactly (itkBinaryMorphologicalClosingImageFilter.hxx:56-62):
    //    the background is 0, switched to the type max iff the foreground is 0 so the two values stay DISTINCT.
    //    (ITK's line 56-62 is a local override of that filter's background; do NOT "simplify" it to the base
    //    class's NonpositiveMin default, which would collapse onto the foreground for an unsigned type with
    //    foreground 0.) The derived value is 0 or the type max, both always representable, so it needs no range
    //    check; it is also the SafeBorder pad constant. Because this matches ITK, Closing is legacy-parity tested
    //    for foreground 0 as well (see BinaryMorphologicalClosingImageFilterTest).
    const T bgT = (op == BinaryCompositeOp::Opening) ? static_cast<T>(background) : ((fgT == T{0}) ? std::numeric_limits<T>::max() : T{0});

    // VALIDATE-ONCE: run the binary-input safeguard EXACTLY ONCE on the ORIGINAL input, then drive the engine
    // directly on the scratch/out stores below -- the {fg, bg} intermediate is binary by construction and must
    // not be re-scanned (going through ExecuteBinaryMorphologyImageFilter per sub-op would re-scan it).
    if(Result<> r = ScanBinaryInput<T>(inStore, fgT, bgT, inputArrayPath, shouldCancel); r.invalid())
    {
      return r;
    }

    const std::string scratchFormat = inStore.getStoreType() == IDataStore::StoreType::OutOfCore  ? inArray.getDataFormat() :
                                      outStore.getStoreType() == IDataStore::StoreType::OutOfCore ? outArray.getDataFormat() :
                                                                                                    inArray.getDataFormat();

    // Bounded-memory ring pipeline (see ApplyBinaryMorphologyCompositePipeline) for an active OOC endpoint,
    // reserving its bounded working-memory requirement up front; a reservation shortfall or a purely in-core
    // run (no active OOC endpoint) falls back to the chained two-pass composition below unchanged, so in-core
    // behavior stays byte- and allocation-identical to today. @p pipelineSafeBorder is passed separately from
    // the struct's @c safeBorder field because Opening always calls this with @c false (ITK's binary Opening
    // has no SafeBorder), regardless of the requested value.
    const auto tryBinaryMorphologyPipeline = [&](MorphOp firstOp, MorphOp secondOp, bool pipelineSafeBorder) -> std::optional<Result<>> {
      if(!hasActiveOocEndpoint)
      {
        return std::nullopt;
      }
      auto requiredBytesResult = CalculateBinaryMorphologyCompositePipelineWorkingMemoryBytes<T>(dims, se, pipelineSafeBorder);
      if(requiredBytesResult.invalid())
      {
        return std::nullopt;
      }
      auto reservation = ReserveWorkingMemory(requiredBytesResult.value(), requiredBytesResult.value());
      if(reservation.sizeBytes() != requiredBytesResult.value())
      {
        return std::nullopt;
      }
      return ApplyBinaryMorphologyCompositePipeline<T>(inStore, outStore, dims, se, firstOp, secondOp, pipelineSafeBorder, fgT, bgT, shouldCancel, static_cast<usize>(reservation.sizeBytes()));
    };

    if(op == BinaryCompositeOp::Opening)
    {
      if(auto result = tryBinaryMorphologyPipeline(MorphOp::Erode, MorphOp::Dilate, /*pipelineSafeBorder=*/false); result.has_value())
      {
        return *result;
      }
      // Opening = erode(boundaryToForeground=true) -> dilate(boundaryToForeground=false). No padding. ONE
      // same-extent scratch: erode(in) -> scratch; dilate(scratch) -> out. Each pass reads a store distinct
      // from the one it writes.
      auto scratch = DataStoreUtilities::CreateDataStoreWithFormat<T>(scratchFormat, inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
      if(Result<> r = ApplyBinaryMorphology<T>(inStore, *scratch, dims, se, MorphOp::Erode, fgT, bgT, /*boundaryToForeground=*/true, inArray, outArray, shouldCancel, messageHandler); r.invalid())
      {
        return r;
      }
      return ApplyBinaryMorphology<T>(*scratch, outStore, dims, se, MorphOp::Dilate, fgT, bgT, /*boundaryToForeground=*/false, inArray, outArray, shouldCancel, messageHandler);
    }

    // Closing = dilate(boundaryToForeground=false) -> erode(boundaryToForeground=true).
    if(auto result = tryBinaryMorphologyPipeline(MorphOp::Dilate, MorphOp::Erode, safeBorder); result.has_value())
    {
      return *result;
    }
    if(!safeBorder)
    {
      // SafeBorder == false: plain two-pass composition on the original extent. ONE same-extent scratch.
      auto scratch = DataStoreUtilities::CreateDataStoreWithFormat<T>(scratchFormat, inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
      if(Result<> r = ApplyBinaryMorphology<T>(inStore, *scratch, dims, se, MorphOp::Dilate, fgT, bgT, /*boundaryToForeground=*/false, inArray, outArray, shouldCancel, messageHandler); r.invalid())
      {
        return r;
      }
      return ApplyBinaryMorphology<T>(*scratch, outStore, dims, se, MorphOp::Erode, fgT, bgT, /*boundaryToForeground=*/true, inArray, outArray, shouldCancel, messageHandler);
    }

    // SafeBorder == true (ITK default): pad the volume by the per-axis kernel radius, fill the border with the
    // background value (ITK's ConstantPad constant), run both passes on the padded volume, then crop the
    // original extent back out. Because we pad by exactly the radius, neither pass reaches the padded edge over
    // the cropped region, so the boundary flags do not affect the cropped result -- reproducing ITK's
    // Pad/dilate/erode/Crop exactly. Two padded ping-pong scratches (see the grayscale SafeBorder path).
    const std::array<usize, 3> r = SafeBorderPadRadius(se, dims);
    const SizeVec3 pdims = {dims[0] + 2 * r[0], dims[1] + 2 * r[1], dims[2] + 2 * r[2]};
    const std::vector<usize> paddedShape = {pdims[2], pdims[1], pdims[0]}; // ZYX tuple shape

    auto paddedIn = DataStoreUtilities::CreateDataStoreWithFormat<T>(scratchFormat, paddedShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
    auto paddedTmp = DataStoreUtilities::CreateDataStoreWithFormat<T>(scratchFormat, paddedShape, std::vector<usize>{1}, IDataAction::Mode::Execute);

    // Boundary flags (dilate false, erode true) match the non-padded path; over the cropped region they do not
    // affect the result (the padded neighborhoods never reach the padded edge there), as noted above.
    if(Result<> r2 = PadIntoStore<T>(inStore, *paddedIn, dims, r, bgT, shouldCancel); r2.invalid())
    {
      return r2;
    }
    if(Result<> r2 = ApplyBinaryMorphology<T>(*paddedIn, *paddedTmp, pdims, se, MorphOp::Dilate, fgT, bgT, false, inArray, outArray, shouldCancel, messageHandler); r2.invalid())
    {
      return r2;
    }
    if(Result<> r2 = ApplyBinaryMorphology<T>(*paddedTmp, *paddedIn, pdims, se, MorphOp::Erode, fgT, bgT, true, inArray, outArray, shouldCancel, messageHandler); r2.invalid())
    {
      return r2;
    }
    return CropFromStore<T>(*paddedIn, outStore, dims, r, shouldCancel);
  }
};

template <template <class> class OutTypeMap, class OperationT>
struct HistogramExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  const OperationT& operation;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    using U = OutTypeMap<T>; // Otsu uses AlwaysUInt8 (a uint8 label image); generalized like the other façade helpers.
    const auto& inStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outStore = dataStructure.getDataRefAs<DataArray<U>>(outputArrayPath).getDataStoreRef();

    Result<Histogram> hist = [&]() -> Result<Histogram> {
      if constexpr(std::is_integral_v<T>)
      {
        return ComputeIntegralHistogram<T>(inStore, operation.numBins, shouldCancel);
      }
      else
      {
        // Only min/max feed the histogram range below, so the parallel min/max reduction replaces the full
        // (mean/variance-computing) statistics pass.
        Result<ArrayMinMax<T>> stats = ComputeArrayMinMax<T>(inStore, shouldCancel);
        if(stats.invalid())
        {
          return {nonstd::make_unexpected(std::move(stats.errors()))};
        }
        if(shouldCancel)
        {
          return {};
        }
        return ComputeHistogram<T>(inStore, operation.numBins, static_cast<float64>(stats.value().min), static_cast<float64>(stats.value().max), shouldCancel);
      }
    }();
    if(hist.invalid())
    {
      return {nonstd::make_unexpected(std::move(hist.errors()))};
    }
    if(shouldCancel)
    {
      return {};
    }

    const std::vector<float64> thresholds = OtsuMultipleThresholds(hist.value(), operation.numThresholds, operation.valleyEmphasis, operation.returnBinMidpoint);
    auto classifyOp = operation.template makeClassifyOp<T, U>(thresholds);
    return ApplyPointwise<T, U>(inStore, outStore, classifyOp, shouldCancel, messageHandler);
  }
};

template <template <class> class OutTypeMap, class ReduceFn>
struct AxisProjectionExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  usize projDim;
  const ReduceFn& reduce;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    // Output element type is the input type mapped through OutTypeMap (T for Max; a float for Mean/Sum/...).
    // Must match the type PreflightAxisProjection<OutTypeMap> allocated, or the getDataRefAs below throws.
    using U = OutTypeMap<T>;
    const auto& inputStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outputStore = dataStructure.getDataRefAs<DataArray<U>>(outputArrayPath).getDataStoreRef();
    return ApplyAxisProjection<T, U, ReduceFn>(inputStore, outputStore, dims, projDim, reduce, shouldCancel, messageHandler);
  }
};

/**
 * @brief Which marker image a grayscale reconstruction filter derives from its single input, matching the ITK
 *        consumer it replaces:
 *          - FillholeMaxBorder: the input's global maximum in the interior and the input value on the effective-
 *            dimensionality border (ITK GrayscaleFillholeImageFilter marker); reconstruct-by-erosion.
 *          - GrindPeakMinBorder: the input's global minimum in the interior and the input value on the effective-
 *            dimensionality border (ITK GrayscaleGrindPeakImageFilter marker); reconstruct-by-dilation. Dual of
 *            FillholeMaxBorder (min interior + dilation instead of max interior + erosion).
 *          - MinusHeight: saturating (input - height) (ITK HMaximaImageFilter marker); reconstruct-by-dilation.
 *          - PlusHeight:  saturating (input + height) (ITK HMinimaImageFilter marker); reconstruct-by-erosion.
 */
enum class ReconMarker
{
  FillholeMaxBorder,
  GrindPeakMinBorder,
  MinusHeight,
  PlusHeight
};

/**
 * @brief Build the reconstruction marker for @p kind into @p markerStore from @p inStore. Streams so peak memory
 *        stays bounded (a fixed chunk for the height markers, one z-plane for Fillhole), so an out-of-core input
 *        yields an out-of-core-bounded marker. @p dims is (X,Y,Z); @p height feeds MinusHeight/PlusHeight only.
 *
 * MinusHeight/PlusHeight offset every voxel by +/-height through @ref SaturateCastFromDouble, so an offset that
 * leaves the type range saturates (ITK-faithful clamp) instead of wrapping.
 *
 * The Fillhole/GrindPeak marker border is the boundary in ITK's EFFECTIVE dimensionality: the legacy filter
 * processes a (W,H,1) volume as a 2D ITK image (ITKArrayHelper selects Dimension 2 when Z==1), so X and Y are always
 * real axes (border at their extremes) while Z is a border axis only when it is not collapsed (dimZ>1). The border
 * carries the input value; the interior carries the global max (Fillhole, reconstruct-by-erosion → fills dark holes)
 * or the global min (GrindPeak, reconstruct-by-dilation → grinds bright peaks). This reproduces ITK's inset-region
 * rule, including "needle" geometries where a non-collapsed axis has extent 1 (every voxel is then a border voxel
 * and the image is left unchanged).
 *
 * @pre @p markerStore has the same tuple/element layout as @p inStore (X fastest). @tparam T scalar type.
 */
template <class T>
Result<> BuildReconstructionMarker(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& markerStore, const SizeVec3& dims, ReconMarker kind, float64 height, const std::atomic_bool& shouldCancel)
{
  if(kind == ReconMarker::MinusHeight || kind == ReconMarker::PlusHeight)
  {
    const float64 signedHeight = (kind == ReconMarker::MinusHeight) ? -height : height;
    const usize total = inStore.getSize();
    const usize chunkValues = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || markerStore.getStoreType() == IDataStore::StoreType::OutOfCore ?
                                  std::max<usize>(1, (64ULL * 1024ULL * 1024ULL) / sizeof(T)) :
                                  65536;
    auto buffer = std::make_unique<T[]>(std::min(chunkValues, total));
    for(usize start = 0; start < total; start += chunkValues)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(chunkValues, total - start);
      if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
      {
        return r;
      }
      for(usize i = 0; i < count; ++i)
      {
        buffer[i] = SaturateCastFromDouble<T>(static_cast<float64>(buffer[i]) + signedHeight);
      }
      if(Result<> r = markerStore.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
      {
        return r;
      }
    }
    return {};
  }

  // FillholeMaxBorder / GrindPeakMinBorder: the input value on the effective-dimensionality border, and a uniform
  // interior fill -- the global MAX for Fillhole (reconstruct-by-erosion fills dark holes), the global MIN for
  // GrindPeak (reconstruct-by-dilation grinds bright peaks). The border streaming below is identical for both.
  // Only min/max are consumed, so the parallel min/max reduction replaces the full statistics pass.
  Result<ArrayMinMax<T>> statsResult = ComputeArrayMinMax<T>(inStore, shouldCancel);
  if(statsResult.invalid())
  {
    return {nonstd::make_unexpected(std::move(statsResult.errors()))};
  }
  if(shouldCancel)
  {
    return {};
  }
  const T interiorFill = (kind == ReconMarker::FillholeMaxBorder) ? statsResult.value().max : statsResult.value().min;

  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);
  if(nX <= 0 || nY <= 0 || nZ <= 0)
  {
    return {};
  }
  const usize dimX = static_cast<usize>(nX);
  const usize dimY = static_cast<usize>(nY);
  const usize dimZ = static_cast<usize>(nZ);
  if(inStore.getStoreType() != IDataStore::StoreType::OutOfCore && markerStore.getStoreType() != IDataStore::StoreType::OutOfCore)
  {
    const usize planeSize = dimX * dimY;
    auto inputPlane = std::make_unique<T[]>(planeSize);
    auto markerPlane = std::make_unique<T[]>(planeSize);
    for(usize z = 0; z < dimZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize planeStart = z * planeSize;
      if(Result<> result = inStore.copyIntoBuffer(planeStart, nonstd::span<T>(inputPlane.get(), planeSize)); result.invalid())
      {
        return result;
      }
      const bool zBorder = dimZ > 1 && (z == 0 || z + 1 == dimZ);
      for(usize y = 0; y < dimY; ++y)
      {
        const bool yBorder = y == 0 || y + 1 == dimY;
        for(usize x = 0; x < dimX; ++x)
        {
          const usize index = y * dimX + x;
          const bool border = x == 0 || x + 1 == dimX || yBorder || zBorder;
          markerPlane[index] = border ? inputPlane[index] : interiorFill;
        }
      }
      if(Result<> result = markerStore.copyFromBuffer(planeStart, nonstd::span<const T>(markerPlane.get(), planeSize)); result.invalid())
      {
        return result;
      }
    }
    return {};
  }
  const usize total = inStore.getSize();
  constexpr usize k_TargetBytes = 64ULL * 1024ULL * 1024ULL;
  constexpr usize k_ChunkValues = std::max<usize>(1, k_TargetBytes / sizeof(T));
  auto buffer = std::make_unique<T[]>(std::min(k_ChunkValues, total));
  for(usize start = 0; start < total; start += k_ChunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkValues, total - start);
    if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
    {
      return r;
    }
    for(usize index = 0; index < count; ++index)
    {
      const usize globalIndex = start + index;
      const usize x = globalIndex % dimX;
      const usize yz = globalIndex / dimX;
      const usize y = yz % dimY;
      const usize z = yz / dimY;
      const bool zBorder = (dimZ > 1) && (z == 0 || z + 1 == dimZ);
      const bool yBorder = (y == 0 || y + 1 == dimY);
      const bool border = (x == 0 || x + 1 == dimX) || yBorder || zBorder;
      buffer[index] = border ? buffer[index] : interiorFill;
    }
    if(Result<> r = markerStore.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

/**
 * @brief Dispatched execute body for the three grayscale reconstruction filters. Derives the marker (@ref
 *        BuildReconstructionMarker) into an out-of-core-inheriting scratch store, then reconstructs it under the
 *        original input (the mask) into the output. The engine's DispatchAlgorithm keys the in-core/out-of-core
 *        path on the real input + output arrays (the scratch marker inherits the input's storage mode), so an
 *        out-of-core input drives the streamed sweep end-to-end. Output type == input type (SameAsInput).
 */
struct MorphologicalReconstructionExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  ReconstructOp op;
  bool fullyConnected;
  ReconMarker markerKind;
  float64 height;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;
  bool differenceFromInput; // HConvex: after reconstruction, output = input - reconstruction (the h-dome)

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<T>& outStore = outArray.getDataStoreRef();

    std::shared_ptr<AbstractDataStore<T>> markerStore;
    const AbstractDataStore<T>* marker = &inStore;
    ReconstructionMarkerOptions markerOptions;
    const bool isHeightMarker = markerKind == ReconMarker::MinusHeight || markerKind == ReconMarker::PlusHeight;
    const bool isBorderMarker = markerKind == ReconMarker::FillholeMaxBorder || markerKind == ReconMarker::GrindPeakMinBorder;
    const bool useDerivedMarker = isBorderMarker || (inStore.getStoreType() == IDataStore::StoreType::OutOfCore && isHeightMarker);
    if(useDerivedMarker)
    {
      if(markerKind == ReconMarker::MinusHeight || markerKind == ReconMarker::PlusHeight)
      {
        markerOptions.source = markerKind == ReconMarker::MinusHeight ? ReconstructionMarkerSource::MaskMinusHeight : ReconstructionMarkerSource::MaskPlusHeight;
      }
      else
      {
        markerOptions.source = markerKind == ReconMarker::FillholeMaxBorder ? ReconstructionMarkerSource::MaskWithMaximumInterior : ReconstructionMarkerSource::MaskWithMinimumInterior;
      }
      markerOptions.height = height;
    }
    else
    {
      // Marker scratch inherits the input's storage format (out-of-core input -> out-of-core scratch), keeping the
      // whole filter memory-bounded through the streamed sweep path.
      markerStore = DataStoreUtilities::CreateDataStoreWithFormat<T>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
      if(Result<> r = BuildReconstructionMarker<T>(inStore, *markerStore, dims, markerKind, height, shouldCancel); r.invalid())
      {
        return r;
      }
      marker = markerStore.get();
    }
    // Mask == the ORIGINAL input; the engine reconstructs the marker under it into outStore. OOC detection uses
    // the real input (mask) + output arrays (the scratch marker has no IDataArray).
    const bool useTemporaryWork = markerKind == ReconMarker::FillholeMaxBorder || markerKind == ReconMarker::GrindPeakMinBorder;
    if(Result<> r = ApplyMorphologicalReconstruction<T>(*marker, inStore, outStore, dims, op, fullyConnected, inArray, outArray, shouldCancel, messageHandler, useTemporaryWork, markerOptions);
       r.invalid())
    {
      return r;
    }
    if(differenceFromInput)
    {
      // HConvex: output = input - reconstruction (the h-maxima result), i.e. the h-dome. SubtractStores is
      // alias-safe (out == b), and hmax <= input everywhere so the difference never underflows.
      return SubtractStores<T>(inStore, outStore, outStore, shouldCancel);
    }
    return {};
  }
};

/**
 * @brief Streamed compare-select for the PreserveIntensities pass of opening/closing-by-reconstruction: for each
 *        voxel, out[i] = (morph[i] == recon[i]) ? input[i] : unmarkedFill. Where the first reconstruction left the
 *        morphology result unchanged, the ORIGINAL input intensity is re-seeded; everywhere else is set to the
 *        neutral extremum (type lowest for opening, type max for closing) so the second reconstruction propagates
 *        only the preserved intensities. Bounded memory (three chunk buffers), out-of-core-safe.
 */
template <class T>
Result<> BuildPreserveIntensitiesMarker(const AbstractDataStore<T>& morph, const AbstractDataStore<T>& recon, const AbstractDataStore<T>& input, AbstractDataStore<T>& out, T unmarkedFill,
                                        const std::atomic_bool& shouldCancel)
{
  const usize size = morph.getSize();
  constexpr usize k_ChunkValues = std::max<usize>(1, (16ULL * 1024ULL * 1024ULL) / (3 * sizeof(T)));
  auto bufMorph = std::make_unique<T[]>(std::min(k_ChunkValues, size));
  auto bufRecon = std::make_unique<T[]>(std::min(k_ChunkValues, size));
  auto bufInput = std::make_unique<T[]>(std::min(k_ChunkValues, size));
  for(usize start = 0; start < size; start += k_ChunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkValues, size - start);
    if(Result<> r = morph.copyIntoBuffer(start, nonstd::span<T>(bufMorph.get(), count)); r.invalid())
    {
      return r;
    }
    if(Result<> r = recon.copyIntoBuffer(start, nonstd::span<T>(bufRecon.get(), count)); r.invalid())
    {
      return r;
    }
    if(Result<> r = input.copyIntoBuffer(start, nonstd::span<T>(bufInput.get(), count)); r.invalid())
    {
      return r;
    }
    for(usize i = 0; i < count; ++i)
    {
      bufMorph[i] = (bufMorph[i] == bufRecon[i]) ? bufInput[i] : unmarkedFill;
    }
    if(Result<> r = out.copyFromBuffer(start, nonstd::span<const T>(bufMorph.get(), count)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

/**
 * @brief Dispatched execute body for opening/closing-by-reconstruction. @p firstOp == Erode -> Opening (erode then
 *        reconstruct-by-dilation); Dilate -> Closing (dilate then reconstruct-by-erosion). The erode/dilate reuses
 *        the standalone ApplyMorphology (skip-OOB), matching ITK's internal GrayscaleErode/Dilate. All buffers are
 *        OOC-inheriting scratch stores, so the whole chain stays memory-bounded; OOC detection uses the real
 *        input/output arrays. Output type == input type.
 */
struct ReconstructionOpenCloseExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  const StructuringElement& se;
  MorphOp firstOp;
  bool fullyConnected;
  bool preserveIntensities;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<T>& outStore = outArray.getDataStoreRef();

    const ReconstructOp reconOp = (firstOp == MorphOp::Erode) ? ReconstructOp::Dilation : ReconstructOp::Erosion;

    // Step 1: grayscale erode (Opening) / dilate (Closing) into an OOC-inheriting scratch.
    auto morph = DataStoreUtilities::CreateDataStoreWithFormat<T>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    if(Result<> r = ApplyMorphology<T>(inStore, *morph, dims, se, firstOp, inArray, outArray, shouldCancel, messageHandler); r.invalid())
    {
      return r;
    }

    if(!preserveIntensities)
    {
      // Reconstruct the morphology result under the input directly into the output.
      return ApplyMorphologicalReconstruction<T>(*morph, inStore, outStore, dims, reconOp, fullyConnected, inArray, outArray, shouldCancel, messageHandler);
    }

    // PreserveIntensities: reconstruct into a scratch, re-seed the original input where the first reconstruction was
    // a fixpoint of the morphology result, then reconstruct that marker into the output.
    auto recon = DataStoreUtilities::CreateDataStoreWithFormat<T>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    if(Result<> r = ApplyMorphologicalReconstruction<T>(*morph, inStore, *recon, dims, reconOp, fullyConnected, inArray, outArray, shouldCancel, messageHandler); r.invalid())
    {
      return r;
    }
    // Build the compare-select marker in place into *recon, aliased as both the read-only reconstruction operand and
    // the output. BuildPreserveIntensitiesMarker copies each chunk into local buffers before writing that chunk back,
    // so the aliasing is safe (reads precede the write per chunk) and it avoids a third full-volume scratch. Neither
    // *morph nor the pre-marker contents of *recon are needed after this pass.
    const T unmarkedFill = (firstOp == MorphOp::Erode) ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
    if(Result<> r = BuildPreserveIntensitiesMarker<T>(*morph, *recon, inStore, *recon, unmarkedFill, shouldCancel); r.invalid())
    {
      return r;
    }
    return ApplyMorphologicalReconstruction<T>(*recon, inStore, outStore, dims, reconOp, fullyConnected, inArray, outArray, shouldCancel, messageHandler);
  }
};

/**
 * @brief Dispatched execute body for binary opening by reconstruction: a binary erosion followed by a binary
 *        reconstruction-by-dilation (retain the foreground components of the input that survive the erosion). The
 *        erode runs on the shipped binary morphology engine; the binary reconstruction is expressed on the shipped
 *        grayscale reconstruction engine by choosing the op from the fg/bg ordering (fg>bg -> Dilation propagates
 *        the high foreground; fg<bg -> Erosion propagates the low foreground). Integer element types only.
 */
struct BinaryReconstructionOpeningExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  const StructuringElement& se;
  float64 foreground;
  float64 background;
  bool fullyConnected;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<T>& outStore = outArray.getDataStoreRef();

    const T foregroundT = static_cast<T>(foreground);
    const T backgroundT = static_cast<T>(background);

    // Enforce the strictly-{fg, bg} binary-input contract (as the standalone binary morphology filters do).
    if(Result<> r = ScanBinaryInput<T>(inStore, foregroundT, backgroundT, inputArrayPath, shouldCancel); r.invalid())
    {
      return r;
    }

    // Binary erosion into an OOC-inheriting scratch. boundaryToForeground=true matches ITK's internal BinaryErode.
    auto erode = DataStoreUtilities::CreateDataStoreWithFormat<T>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    if(Result<> r = ApplyBinaryMorphology<T>(inStore, *erode, dims, se, MorphOp::Erode, foregroundT, backgroundT, /*boundaryToForeground=*/true, inArray, outArray, shouldCancel, messageHandler);
       r.invalid())
    {
      return r;
    }

    // Binary reconstruction-by-dilation via the grayscale engine: the op makes "foreground" the propagated extreme,
    // so a foreground component survives iff it contains an eroded (marker) foreground voxel. For the ordinary
    // (center-including) kernels the erode is a subset of the object, so the reconstruction direction precondition
    // holds directly (Dilation: marker<=mask; Erosion: marker>=mask). The center-EXCLUDING Annulus kernel can erode
    // a background hole to foreground (marker > mask there), technically violating that precondition; the result is
    // still correct because the engine's clampToMask intersects such a stray marker back to the mask on the first
    // pass -- matching ITK's binary-reconstruction labelizer, which likewise ignores out-of-mask seeds. (See the
    // Annulus note in @ref MakeStructuringElement / the erode path.)
    const ReconstructOp op = (foregroundT > backgroundT) ? ReconstructOp::Dilation : ReconstructOp::Erosion;
    return ApplyMorphologicalReconstruction<T>(*erode, inStore, outStore, dims, op, fullyConnected, inArray, outArray, shouldCancel, messageHandler);
  }
};

/**
 * @brief Dispatched execute body for the valued regional-extrema filters (ValuedRegionalMaxima/Minima): runs the
 *        D3-split regional-extrema engine into the output (regional extrema keep their value, everything else the
 *        marker value). Output type == input type.
 */
struct ValuedRegionalExtremaExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  RegionalExtremaOp op;
  bool fullyConnected;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    return ApplyValuedRegionalExtrema<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, op, fullyConnected, inArray, outArray, shouldCancel, messageHandler);
  }
};

/**
 * @brief Fill an entire data store with a single value, streamed in bounded chunks (out-of-core-safe).
 */
template <class T>
Result<> FillStore(AbstractDataStore<T>& out, T value, const std::atomic_bool& shouldCancel)
{
  const usize total = out.getSize();
  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<T[]>(std::min(k_ChunkValues, total));
  std::fill(buffer.get(), buffer.get() + std::min(k_ChunkValues, total), value);
  for(usize start = 0; start < total; start += k_ChunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkValues, total - start);
    if(Result<> r = out.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

/**
 * @brief Dispatched execute body for the binary-output regional-extrema filters (RegionalMaxima/Minima). Computes
 *        the valued regional extrema into a scratch, then thresholds it: pixels left at the marker value (the
 *        non-extrema) become @c background, pixels that kept their value (the extrema) become @c foreground. A flat
 *        image is short-circuited to all-foreground (if @c flatIsExtremum) or all-background, matching ITK's
 *        GetFlat() branch. The output is a fixed uint32 label image (matching the legacy ITK FilterOutputType), not
 *        the input element type.
 */
struct RegionalExtremaExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  RegionalExtremaOp op;
  bool fullyConnected;
  float64 foreground;
  float64 background;
  bool flatIsExtremum;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    // Input is the T-typed image; the output is a FIXED uint32 binary label image (matching the legacy ITK
    // RegionalMaxima/Minima FilterOutputType), so the preflight must have created it via the AlwaysUInt32 map.
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<uint32>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<uint32>& outStore = outArray.getDataStoreRef();
    const uint32 fgU = static_cast<uint32>(foreground);
    const uint32 bgU = static_cast<uint32>(background);

    // Flat-image special case (ITK's GetFlat() branch): a completely flat image has no regional structure, so the
    // whole image is either the extremum (all foreground) or not (all background), per flatIsExtremum. Only
    // min/max are consumed, so the parallel min/max reduction replaces the full statistics pass.
    Result<ArrayMinMax<T>> stats = ComputeArrayMinMax<T>(inStore, shouldCancel);
    if(stats.invalid())
    {
      return {nonstd::make_unexpected(std::move(stats.errors()))};
    }
    if(shouldCancel)
    {
      return {};
    }
    if(stats.value().min == stats.value().max)
    {
      return FillStore<uint32>(outStore, flatIsExtremum ? fgU : bgU, shouldCancel);
    }

    // Compute the valued regional extrema into an OOC-inheriting scratch (input type T), then threshold that into
    // the uint32 output: non-extrema (left at the marker value) -> background, extrema (kept value) -> foreground.
    auto scratch = DataStoreUtilities::CreateDataStoreWithFormat<T>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    if(Result<> r = ApplyValuedRegionalExtrema<T>(inStore, *scratch, dims, op, fullyConnected, inArray, outArray, shouldCancel, messageHandler); r.invalid())
    {
      return r;
    }
    const T markerValue = (op == RegionalExtremaOp::Maxima) ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
    const usize total = inStore.getSize();
    constexpr usize k_ChunkValues = std::max<usize>(1, (16ULL * 1024ULL * 1024ULL) / (sizeof(T) + sizeof(uint32)));
    auto scratchBuf = std::make_unique<T[]>(std::min(k_ChunkValues, total));
    auto outBuf = std::make_unique<uint32[]>(std::min(k_ChunkValues, total));
    for(usize start = 0; start < total; start += k_ChunkValues)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_ChunkValues, total - start);
      if(Result<> r = scratch->copyIntoBuffer(start, nonstd::span<T>(scratchBuf.get(), count)); r.invalid())
      {
        return r;
      }
      for(usize i = 0; i < count; ++i)
      {
        outBuf[i] = (scratchBuf[i] == markerValue) ? bgU : fgU;
      }
      if(Result<> r = outStore.copyFromBuffer(start, nonstd::span<const uint32>(outBuf.get(), count)); r.invalid())
      {
        return r;
      }
    }
    return {};
  }
};

/**
 * @brief Dispatched execute body for the Signed Maurer Distance Map filter. The input is the T-typed (integer scalar)
 *        image; the output is a FIXED float32 signed distance image (matching the legacy ITK SignedMaurerDistanceMap
 *        FilterOutputType), so the preflight must have created it via the AlwaysFloat32 map. The Float64 BackgroundValue
 *        is cast to the input element type T here; the caller's preflight has enforced an integer scalar input.
 */
struct SignedMaurerDistanceMapExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  bool insideIsPositive;
  bool squaredDistance;
  bool useSpacing;
  FloatVec3 spacing;
  float64 backgroundValue;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    // The in/out IDataArrays are needed by ApplySignedMaurerDistanceMap's DispatchAlgorithm to decide the storage
    // path; the typed stores are the buffers the engine reads/writes. Output type is a FIXED float32 signed distance
    // image (matching the legacy ITK FilterOutputType), not the input element type.
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputArrayPath);
    return ApplySignedMaurerDistanceMap<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, static_cast<T>(backgroundValue), insideIsPositive, squaredDistance, useSpacing, spacing,
                                           inArray, outArray, shouldCancel, messageHandler);
  }
};

/**
 * @brief Dispatched execute body for the (unsigned) Danielsson Distance Map filter. The input is the T-typed (integer
 *        scalar) image; the output is a FIXED float32 distance image (matching the legacy ITK Danielsson
 *        FilterOutputType), so the preflight must have created it via the AlwaysFloat32 map. @p inputIsBinary is
 *        carried for interface/SIMPL parity but does not affect the distance output.
 */
struct DanielssonDistanceMapExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  bool inputIsBinary;
  bool squaredDistance;
  bool useSpacing;
  FloatVec3 spacing;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputArrayPath);
    return ApplyDanielssonDistanceMap<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, inputIsBinary, squaredDistance, useSpacing, spacing, inArray, outArray, shouldCancel,
                                         messageHandler);
  }
};

/**
 * @brief Dispatched execute body for the Signed Danielsson Distance Map filter -- the ITK composite (verified in
 *        itkSignedDanielssonDistanceMapImageFilter.hxx): d1 = Danielsson(input); d2 = Danielsson(BinaryDilate(invert
 *        (input), ball radius 1)); output = InsideIsPositive ? (d2 - d1) : (d1 - d2). All work runs on scratch stores
 *        using an OOC endpoint's storage format whenever one is available.
 */
struct SignedDanielssonDistanceMapExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  bool insideIsPositive;
  bool squaredDistance;
  bool useSpacing;
  FloatVec3 spacing;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<float32>& outStore = outArray.getDataStoreRef();

    // Scratch stores prefer an OOC input format, then an OOC output format, and otherwise preserve the input format.
    // This keeps every full-volume intermediate disk-backed for mixed/adaptive executions. The first Danielsson pass
    // (d1) is written directly into the real float32 output store and the result is subtracted IN PLACE, so no separate
    // d1 scratch is needed; peak scratch is one T image (dil; inv is freed after the dilate) plus one float32 image (d2).
    const std::string format = detail::SelectDanielssonWorkingDataFormat(inStore.getStoreType(), inStore.getDataFormat(), outStore.getStoreType(), outStore.getDataFormat());
    const std::vector<usize> tupleShape = inArray.getTupleShape();
    auto invStore = DataStoreUtilities::CreateDataStoreWithFormat<T>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
    auto dilStore = DataStoreUtilities::CreateDataStoreWithFormat<T>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
    auto d2Store = DataStoreUtilities::CreateDataStoreWithFormat<float32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);

    // (1) inv = InvertIntensity(input): (input != 0) ? 0 : 1.
    if(Result<> r = InvertBinaryStore<T>(inStore, *invStore, shouldCancel); r.invalid())
    {
      return r;
    }
    // (2) dil = BinaryDilate(inv, ball radius 1, DilateValue=1). ITK's BinaryBallStructuringElement(1) is
    // FlatStructuringElement::Ball(1) (== KernelType::Ball); outside is background (boundaryToForeground=false).
    const StructuringElement se = MakeStructuringElement(KernelType::Ball, {1, 1, 1});
    if(Result<> r = ApplyBinaryMorphology<T>(*invStore, *dilStore, dims, se, MorphOp::Dilate, T{1}, T{0}, /*boundaryToForeground=*/false, inArray, outArray, shouldCancel, messageHandler); r.invalid())
    {
      return r;
    }
    invStore.reset(); // no longer needed once the dilate has consumed it

    // (3) d1 = Danielsson(input) -> the real output store; (4) d2 = Danielsson(dil) -> d2 scratch. Both inherit
    // SquaredDistance/UseImageSpacing (ITK sets these on both inner filters); InputIsBinary is left default (it never
    // affects the distance).
    if(Result<> r = ApplyDanielssonDistanceMap<T>(inStore, outStore, dims, /*inputIsBinary=*/false, squaredDistance, useSpacing, spacing, inArray, outArray, shouldCancel, messageHandler); r.invalid())
    {
      return r;
    }
    if(Result<> r = ApplyDanielssonDistanceMap<T>(*dilStore, *d2Store, dims, /*inputIsBinary=*/false, squaredDistance, useSpacing, spacing, inArray, outArray, shouldCancel, messageHandler);
       r.invalid())
    {
      return r;
    }
    // (5) subtract IN PLACE per InsideIsPositive (SubtractStores(a, b, out) = a - b, alias-safe; outStore holds d1):
    // InsideIsPositive -> out = d2 - d1; else -> out = d1 - d2.
    if(insideIsPositive)
    {
      return SubtractStores<float32>(*d2Store, outStore, outStore, shouldCancel);
    }
    return SubtractStores<float32>(outStore, *d2Store, outStore, shouldCancel);
  }
};

/**
 * @brief Dispatched execute body for the IsoContourDistance filter. Input is the T-typed (any scalar) image; output is
 *        a FIXED float32 narrow-band signed distance image (matching the legacy ITK FilterOutputType), so the
 *        preflight must have created it via the AlwaysFloat32 map.
 */
struct IsoContourDistanceExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  float64 levelSetValue;
  float64 farValue;
  FloatVec3 spacing;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputArrayPath);
    return ApplyIsoContourDistance<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, levelSetValue, farValue, spacing, shouldCancel, messageHandler);
  }
};

/**
 * @brief 3-D out-of-core route for GradientMagnitudeRecursiveGaussian. All three axis cascades below read and write
 *        @p rawAccumulator, not @p accumulator: every Set/Accumulate call and the dim-2 terminal sink (which fuses
 *        the final square root) target the raw scratch, so the real output store is touched only once, at the very
 *        end, when the finished values are streamed into it via MaterializeGaussianTemporaryStore. @p AccStoreT is a
 *        template parameter (rather than a fixed GaussianTemporaryStore<float32>) so the working-data-format route
 *        can pass @p accumulator itself as @p rawAccumulator -- in that case AccStoreT is AbstractDataStore<float32>,
 *        the compile-time check below skips the redundant final copy, and behavior is identical to writing directly
 *        into @p accumulator throughout.
 */
template <class T, class WorkStoreT, class AccStoreT>
Result<> ApplyGradientMagnitudeRecursiveGaussian3D(const AbstractDataStore<T>& input, AbstractDataStore<float32>& accumulator, AccStoreT& rawAccumulator, WorkStoreT& work, const SizeVec3& dims,
                                                   const FloatVec3& spacing, float64 sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel,
                                                   const IFilter::MessageHandler& messageHandler)
{
  for(uint32 dim = 0; dim < 3; ++dim)
  {
    if(shouldCancel)
    {
      return {};
    }
    std::vector<float32> accumulatorPlane(dim == 2 ? dims[0] * dims[1] : 0);
    auto terminalSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) -> Result<> {
      const double derivativeSpacing = static_cast<double>(spacing[dim]);
      if(dim == 0)
      {
        for(float32& value : filteredPlane)
        {
          const double derivative = static_cast<double>(value) / derivativeSpacing;
          value = static_cast<float32>(derivative * derivative);
        }
        return rawAccumulator.copyFromBuffer(planeOffset, nonstd::span<const float32>(filteredPlane.data(), filteredPlane.size()));
      }
      if(Result<> result = rawAccumulator.copyIntoBuffer(planeOffset, nonstd::span<float32>(accumulatorPlane.data(), filteredPlane.size())); result.invalid())
      {
        return result;
      }
      for(usize index = 0; index < filteredPlane.size(); ++index)
      {
        const double derivative = static_cast<double>(filteredPlane[index]) / derivativeSpacing;
        accumulatorPlane[index] = static_cast<float32>(static_cast<double>(accumulatorPlane[index]) + derivative * derivative);
        if(dim == 2)
        {
          accumulatorPlane[index] = static_cast<float32>(std::sqrt(static_cast<double>(accumulatorPlane[index])));
        }
      }
      return rawAccumulator.copyFromBuffer(planeOffset, nonstd::span<const float32>(accumulatorPlane.data(), filteredPlane.size()));
    };

    if(dim < 2)
    {
      const uint32 otherAxis = dim == 0 ? 1u : 0u;
      const RecursiveGaussianPlaneAxis derivative{dim, sigma, static_cast<double>(spacing[dim]), 1, normalizeAcrossScale};
      const RecursiveGaussianPlaneAxis smoothing{otherAxis, sigma, static_cast<double>(spacing[otherAxis]), 0, normalizeAcrossScale};
      auto workSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) { return work.copyFromBuffer(planeOffset, nonstd::span<const float32>(filteredPlane.data(), filteredPlane.size())); };
      if(Result<> result = RecursiveGaussianXYPlaneCascadeToSink<T>(input, dims, derivative, smoothing, shouldCancel, messageHandler, workSink); result.invalid())
      {
        return result;
      }
      if(Result<> result = RecursiveGaussianAxisPass<float32>(work, work, dims, 2, sigma, static_cast<double>(spacing[2]), 0, normalizeAcrossScale, shouldCancel, messageHandler); result.invalid())
      {
        return result;
      }
      if(dim == 0)
      {
        if(Result<> result = SetSquaredOverSpacing(rawAccumulator, work, static_cast<double>(spacing[dim]), shouldCancel); result.invalid())
        {
          return result;
        }
      }
      else if(Result<> result = AccumulateSquaredOverSpacing(rawAccumulator, work, static_cast<double>(spacing[dim]), shouldCancel); result.invalid())
      {
        return result;
      }
      continue;
    }

    if(Result<> result = RecursiveGaussianAxisPass<T>(input, work, dims, 2, sigma, static_cast<double>(spacing[2]), 1, normalizeAcrossScale, shouldCancel, messageHandler); result.invalid())
    {
      return result;
    }
    const RecursiveGaussianPlaneAxis smoothX{0, sigma, static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
    const RecursiveGaussianPlaneAxis smoothY{1, sigma, static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
    if(Result<> result = RecursiveGaussianXYPlaneCascadeToSink<float32>(work, dims, smoothX, smoothY, shouldCancel, messageHandler, terminalSink); result.invalid())
    {
      return result;
    }
  }
  if constexpr(!std::is_same_v<AccStoreT, AbstractDataStore<float32>>)
  {
    return MaterializeGaussianTemporaryStore(accumulator, rawAccumulator, shouldCancel);
  }
  else
  {
    return {};
  }
}

/**
 * @brief 3-D out-of-core route for LaplacianRecursiveGaussian. All three axis cascades below read and write
 *        @p rawAccumulator, not @p accumulator: every Set/Accumulate call and the dim-2 terminal sink target the raw
 *        scratch, so the real output store is touched only once, at the very end, when the finished sum of scaled
 *        second derivatives is streamed into it via MaterializeGaussianTemporaryStore. @p AccStoreT is a template
 *        parameter (rather than a fixed GaussianTemporaryStore<float32>) so the working-data-format route can pass
 *        @p accumulator itself as @p rawAccumulator -- in that case AccStoreT is AbstractDataStore<float32>, the
 *        compile-time check below skips the redundant final copy, and behavior is identical to writing directly
 *        into @p accumulator throughout.
 */
template <class T, class WorkStoreT, class AccStoreT>
Result<> ApplyLaplacianRecursiveGaussian3D(const AbstractDataStore<T>& input, AbstractDataStore<float32>& accumulator, AccStoreT& rawAccumulator, WorkStoreT& work, const SizeVec3& dims,
                                           const FloatVec3& spacing, float64 sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  for(uint32 dim = 0; dim < 3; ++dim)
  {
    if(shouldCancel)
    {
      return {};
    }
    const double derivativeSpacing = static_cast<double>(spacing[dim]);
    const double invSpacingSq = 1.0 / (derivativeSpacing * derivativeSpacing);
    std::vector<float32> accumulatorPlane(dim == 2 ? dims[0] * dims[1] : 0);
    auto terminalSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) -> Result<> {
      if(dim == 0)
      {
        for(float32& value : filteredPlane)
        {
          value = static_cast<float32>(static_cast<double>(value) * invSpacingSq);
        }
        return rawAccumulator.copyFromBuffer(planeOffset, nonstd::span<const float32>(filteredPlane.data(), filteredPlane.size()));
      }
      if(Result<> result = rawAccumulator.copyIntoBuffer(planeOffset, nonstd::span<float32>(accumulatorPlane.data(), filteredPlane.size())); result.invalid())
      {
        return result;
      }
      for(usize index = 0; index < filteredPlane.size(); ++index)
      {
        accumulatorPlane[index] = static_cast<float32>(static_cast<double>(accumulatorPlane[index]) + static_cast<double>(filteredPlane[index]) * invSpacingSq);
      }
      return rawAccumulator.copyFromBuffer(planeOffset, nonstd::span<const float32>(accumulatorPlane.data(), filteredPlane.size()));
    };

    if(dim < 2)
    {
      const uint32 otherAxis = dim == 0 ? 1u : 0u;
      const RecursiveGaussianPlaneAxis derivative{dim, sigma, static_cast<double>(spacing[dim]), 2, normalizeAcrossScale};
      const RecursiveGaussianPlaneAxis smoothing{otherAxis, sigma, static_cast<double>(spacing[otherAxis]), 0, normalizeAcrossScale};
      auto workSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) { return work.copyFromBuffer(planeOffset, nonstd::span<const float32>(filteredPlane.data(), filteredPlane.size())); };
      if(Result<> result = RecursiveGaussianXYPlaneCascadeToSink<T>(input, dims, derivative, smoothing, shouldCancel, messageHandler, workSink); result.invalid())
      {
        return result;
      }
      if(Result<> result = RecursiveGaussianAxisPass<float32>(work, work, dims, 2, sigma, static_cast<double>(spacing[2]), 0, normalizeAcrossScale, shouldCancel, messageHandler); result.invalid())
      {
        return result;
      }
      if(dim == 0)
      {
        if(Result<> result = SetScaledDerivative(rawAccumulator, work, invSpacingSq, shouldCancel); result.invalid())
        {
          return result;
        }
      }
      else if(Result<> result = AccumulateScaledDerivative(rawAccumulator, work, invSpacingSq, shouldCancel); result.invalid())
      {
        return result;
      }
      continue;
    }

    if(Result<> result = RecursiveGaussianAxisPass<T>(input, work, dims, 2, sigma, static_cast<double>(spacing[2]), 2, normalizeAcrossScale, shouldCancel, messageHandler); result.invalid())
    {
      return result;
    }
    const RecursiveGaussianPlaneAxis smoothX{0, sigma, static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
    const RecursiveGaussianPlaneAxis smoothY{1, sigma, static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
    if(Result<> result = RecursiveGaussianXYPlaneCascadeToSink<float32>(work, dims, smoothX, smoothY, shouldCancel, messageHandler, terminalSink); result.invalid())
    {
      return result;
    }
  }
  if constexpr(!std::is_same_v<AccStoreT, AbstractDataStore<float32>>)
  {
    return MaterializeGaussianTemporaryStore(accumulator, rawAccumulator, shouldCancel);
  }
  else
  {
    return {};
  }
}

template <class T, class WorkStoreT>
Result<> ApplySmoothingRecursiveGaussian3D(const AbstractDataStore<T>& input, AbstractDataStore<T>& output, WorkStoreT& work, const SizeVec3& dims, const FloatVec3& spacing,
                                           const std::vector<float64>& sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  if(Result<> result = RecursiveGaussianAxisPass<T>(input, work, dims, 2, sigma[2], static_cast<double>(spacing[2]), 0, normalizeAcrossScale, shouldCancel, messageHandler); result.invalid())
  {
    return result;
  }
  std::vector<T> outputPlane(dims[0] * dims[1]);
  auto outputSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) -> Result<> {
    for(usize index = 0; index < filteredPlane.size(); ++index)
    {
      outputPlane[index] = static_cast<T>(filteredPlane[index]);
    }
    return output.copyFromBuffer(planeOffset, nonstd::span<const T>(outputPlane.data(), filteredPlane.size()));
  };
  const RecursiveGaussianPlaneAxis first{0, sigma[0], static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
  const RecursiveGaussianPlaneAxis second{1, sigma[1], static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
  return RecursiveGaussianXYPlaneCascadeToSink<float32>(work, dims, first, second, shouldCancel, messageHandler, outputSink);
}

template <class T, class WorkStoreT, class ContributionStoreT, class CheckpointStoreT>
Result<> ApplyGradientMagnitudeRecursiveGaussian2D(const AbstractDataStore<T>& input, AbstractDataStore<float32>& accumulator, WorkStoreT& work, ContributionStoreT& contribution,
                                                   CheckpointStoreT& checkpoints, const SizeVec3& dims, const FloatVec3& spacing, float64 sigma, bool normalizeAcrossScale,
                                                   const detail::RecursiveGaussian2DBufferPlan& plan, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  {
    auto terminalSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> filtered) -> Result<> {
      const double derivativeSpacing = static_cast<double>(spacing[0]);
      for(float32& value : filtered)
      {
        const double derivative = static_cast<double>(value) / derivativeSpacing;
        value = static_cast<float32>(derivative * derivative);
      }
      return ImageProcessing::WriteRecursiveGaussian2DBlock<float32>(contribution, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(filtered.data(), filtered.size()));
    };
    const RecursiveGaussianPlaneAxis derivative{0, sigma, static_cast<double>(spacing[0]), 1, normalizeAcrossScale};
    const RecursiveGaussianPlaneAxis smoothing{1, sigma, static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
    if(Result<> result = RecursiveGaussian2DXYCascadeToSink<T>(input, work, checkpoints, dims, derivative, smoothing, plan, shouldCancel, messageHandler, terminalSink); result.invalid())
    {
      return result;
    }
  }

  {
    auto terminalSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> filtered, nonstd::span<float32> reusable) -> Result<> {
      const double derivativeSpacing = static_cast<double>(spacing[1]);
      if(Result<> result = detail::ReadRecursiveGaussian2DBlock<float32>(contribution, dims[0], yBegin, rowCount, xBegin, columnCount, reusable); result.invalid())
      {
        return result;
      }
      for(usize index = 0; index < filtered.size(); ++index)
      {
        const double derivative = static_cast<double>(filtered[index]) / derivativeSpacing;
        reusable[index] = static_cast<float32>(static_cast<double>(reusable[index]) + derivative * derivative);
        reusable[index] = static_cast<float32>(std::sqrt(static_cast<double>(reusable[index])));
      }
      return ImageProcessing::WriteRecursiveGaussian2DBlock<float32>(accumulator, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(reusable.data(), reusable.size()));
    };
    const RecursiveGaussianPlaneAxis derivative{1, sigma, static_cast<double>(spacing[1]), 1, normalizeAcrossScale};
    const RecursiveGaussianPlaneAxis smoothing{0, sigma, static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
    if(Result<> result = RecursiveGaussian2DYXCascadeToSink<T>(input, work, checkpoints, dims, derivative, smoothing, plan, shouldCancel, messageHandler, terminalSink); result.invalid())
    {
      return result;
    }
  }
  return {};
}

template <class T, class WorkStoreT, class ContributionStoreT, class CheckpointStoreT>
Result<> ApplyLaplacianRecursiveGaussian2D(const AbstractDataStore<T>& input, AbstractDataStore<float32>& accumulator, WorkStoreT& work, ContributionStoreT& contribution,
                                           CheckpointStoreT& checkpoints, const SizeVec3& dims, const FloatVec3& spacing, float64 sigma, bool normalizeAcrossScale,
                                           const detail::RecursiveGaussian2DBufferPlan& plan, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  {
    auto terminalSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> filtered) -> Result<> {
      const double spacingValue = static_cast<double>(spacing[0]);
      const double invSpacingSq = 1.0 / (spacingValue * spacingValue);
      for(float32& value : filtered)
      {
        value = static_cast<float32>(static_cast<double>(value) * invSpacingSq);
      }
      return ImageProcessing::WriteRecursiveGaussian2DBlock<float32>(contribution, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(filtered.data(), filtered.size()));
    };
    const RecursiveGaussianPlaneAxis derivative{0, sigma, static_cast<double>(spacing[0]), 2, normalizeAcrossScale};
    const RecursiveGaussianPlaneAxis smoothing{1, sigma, static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
    if(Result<> result = RecursiveGaussian2DXYCascadeToSink<T>(input, work, checkpoints, dims, derivative, smoothing, plan, shouldCancel, messageHandler, terminalSink); result.invalid())
    {
      return result;
    }
  }

  {
    auto terminalSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> filtered, nonstd::span<float32> reusable) -> Result<> {
      const double spacingValue = static_cast<double>(spacing[1]);
      const double invSpacingSq = 1.0 / (spacingValue * spacingValue);
      if(Result<> result = detail::ReadRecursiveGaussian2DBlock<float32>(contribution, dims[0], yBegin, rowCount, xBegin, columnCount, reusable); result.invalid())
      {
        return result;
      }
      for(usize index = 0; index < filtered.size(); ++index)
      {
        reusable[index] = static_cast<float32>(static_cast<double>(reusable[index]) + static_cast<double>(filtered[index]) * invSpacingSq);
      }
      return ImageProcessing::WriteRecursiveGaussian2DBlock<float32>(accumulator, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(reusable.data(), reusable.size()));
    };
    const RecursiveGaussianPlaneAxis derivative{1, sigma, static_cast<double>(spacing[1]), 2, normalizeAcrossScale};
    const RecursiveGaussianPlaneAxis smoothing{0, sigma, static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
    if(Result<> result = RecursiveGaussian2DYXCascadeToSink<T>(input, work, checkpoints, dims, derivative, smoothing, plan, shouldCancel, messageHandler, terminalSink); result.invalid())
    {
      return result;
    }
  }
  return {};
}

template <class T, class WorkStoreT, class CheckpointStoreT>
Result<> ApplySmoothingRecursiveGaussian2D(const AbstractDataStore<T>& input, AbstractDataStore<T>& output, WorkStoreT& work, CheckpointStoreT& checkpoints, const SizeVec3& dims,
                                           const FloatVec3& spacing, const std::vector<float64>& sigma, bool normalizeAcrossScale, const detail::RecursiveGaussian2DBufferPlan& plan,
                                           const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  std::vector<T> outputBuffer;
  auto terminalSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> filtered, nonstd::span<float32>) -> Result<> {
    if constexpr(std::is_same_v<T, float32>)
    {
      return ImageProcessing::WriteRecursiveGaussian2DBlock<float32>(output, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(filtered.data(), filtered.size()));
    }
    outputBuffer.resize(filtered.size());
    for(usize index = 0; index < filtered.size(); ++index)
    {
      outputBuffer[index] = static_cast<T>(filtered[index]);
    }
    return ImageProcessing::WriteRecursiveGaussian2DBlock<T>(output, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const T>(outputBuffer.data(), outputBuffer.size()));
  };
  const RecursiveGaussianPlaneAxis smoothY{1, sigma[1], static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
  const RecursiveGaussianPlaneAxis smoothX{0, sigma[0], static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
  return RecursiveGaussian2DYXCascadeToSink<T>(input, work, checkpoints, dims, smoothY, smoothX, plan, shouldCancel, messageHandler, terminalSink);
}

/// @brief Dispatched execute body for the GradientMagnitudeRecursiveGaussian filter.
/// Input is a T-typed scalar image and output is a fixed float32 gradient-magnitude image. For each filtered axis, it
/// differentiates along that axis, smooths along every other axis, and accumulates the squared, spacing-scaled result.
/// Actual OOC endpoints use raw fixed-record float32 scratch; forced plane-cascade tests over resident endpoints retain a
/// format-matched DataStore. Output doubles as the accumulator. This preserves ITK's double-coefficient IIR and float32
/// inter-pass behavior.
struct GradientMagnitudeRecursiveGaussianExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  FloatVec3 spacing;
  float64 sigma;
  bool normalizeAcrossScale;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<float32>& acc = outArray.getDataStoreRef(); // output doubles as the accumulator

    const bool usesOutOfCoreStore = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || acc.getStoreType() == IDataStore::StoreType::OutOfCore;
    const bool usePlaneCascade = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
    RecordAlgorithmPathExecution(usePlaneCascade ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

    const uint32 effDim = (dims[2] > 1) ? 3u : 2u;
    const detail::RecursiveGaussianStoragePlan storagePlan = detail::SelectRecursiveGaussianStoragePlan(inStore.getStoreType(), inStore.getDataFormat(), acc.getStoreType(), acc.getDataFormat());
    if(usePlaneCascade && effDim == 2 && storagePlan.useTemporaryRecordStore)
    {
      detail::RecursiveGaussian2DBufferPlan plan =
          detail::BuildRecursiveGaussian2DBufferPlan(dims[0], dims[1], RecursiveGaussianPassOptions::k_Default2DResidentLimit, std::max<usize>(20, sizeof(T) + 16));
      if(plan.valid && plan.coreCols != dims[0])
      {
        plan = detail::BuildRecursiveGaussian2DBufferPlan(dims[0], dims[1], RecursiveGaussianPassOptions::k_Default2DResidentLimit, std::max<usize>(24, sizeof(T) + 20));
      }
      plan = detail::AlignRecursiveGaussian2DBufferPlanToChunks(plan, dims[0], dims[1], acc.getChunkShape());
      if(plan.overflow || !plan.valid)
      {
        return MakeErrorResult(-23620, fmt::format("Gradient magnitude recursive Gaussian could not build a bounded 2D plan for dimensions {} x {}.", dims[0], dims[1]));
      }
      usize checkpointCount = 0;
      usize workBatchValues = 0;
      if(!detail::RecursiveGaussianCheckedMultiply(plan.blockCount, plan.coreCols, checkpointCount) || !detail::RecursiveGaussianCheckedMultiply(plan.blockRows, plan.coreCols, workBatchValues))
      {
        return MakeErrorResult(-23620, fmt::format("Gradient magnitude recursive Gaussian bounded 2D scratch dimensions overflow for {} x {}.", dims[0], dims[1]));
      }
      checkpointCount = std::max(checkpointCount, detail::RecursiveGaussian2DXBlockCount(dims[0], plan.coreCols));
      auto checkpointResult = detail::CreateGaussianTemporaryStore<detail::RecursiveGaussian2DCheckpoint>(checkpointCount, plan.coreCols, shouldCancel);
      if(checkpointResult.invalid())
      {
        return ConvertResult(std::move(checkpointResult));
      }
      auto workResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), workBatchValues, shouldCancel);
      if(workResult.invalid())
      {
        return ConvertResult(std::move(workResult));
      }
      auto contributionResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), workBatchValues, shouldCancel);
      if(contributionResult.invalid())
      {
        return ConvertResult(std::move(contributionResult));
      }
      std::unique_ptr<detail::GaussianTemporaryStore<detail::RecursiveGaussian2DCheckpoint>> checkpoints = std::move(checkpointResult.value());
      std::unique_ptr<detail::GaussianTemporaryStore<float32>> work = std::move(workResult.value());
      std::unique_ptr<detail::GaussianTemporaryStore<float32>> contribution = std::move(contributionResult.value());
      return ApplyGradientMagnitudeRecursiveGaussian2D<T>(inStore, acc, *work, *contribution, *checkpoints, dims, spacing, sigma, normalizeAcrossScale, plan, shouldCancel, messageHandler);
    }
    if(usePlaneCascade && effDim == 3)
    {
      if(storagePlan.useTemporaryRecordStore)
      {
        auto temporaryResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), dims[0] * dims[1], shouldCancel);
        if(temporaryResult.invalid())
        {
          return ConvertResult(std::move(temporaryResult));
        }
        std::unique_ptr<detail::GaussianTemporaryStore<float32>> work = std::move(temporaryResult.value());
        // Raw (non-deflate-backed) full-volume accumulator scratch, created the same way as `work`: every axis
        // cascade's Set/Accumulate below targets this instead of the real deflate-backed output store, so the
        // running gradient-magnitude sum never inflates/deflates against `acc`. Only the finished, square-rooted
        // result is streamed into `acc` once, at the end of ApplyGradientMagnitudeRecursiveGaussian3D.
        auto accumulatorResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), dims[0] * dims[1], shouldCancel);
        if(accumulatorResult.invalid())
        {
          return ConvertResult(std::move(accumulatorResult));
        }
        std::unique_ptr<detail::GaussianTemporaryStore<float32>> rawAccumulator = std::move(accumulatorResult.value());
        return ApplyGradientMagnitudeRecursiveGaussian3D<T>(inStore, acc, *rawAccumulator, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
      }
      auto work = DataStoreUtilities::CreateDataStoreWithFormat<float32>(storagePlan.workingDataFormat, inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
      return ApplyGradientMagnitudeRecursiveGaussian3D<T>(inStore, acc, acc, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
    }
    if(usePlaneCascade)
    {
      std::vector<float32> accumulatorPlane(dims[0] * dims[1]);
      for(uint32 dim = 0; dim < 2; ++dim)
      {
        if(shouldCancel)
        {
          return {};
        }

        auto terminalSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) -> Result<> {
          const double derivativeSpacing = static_cast<double>(spacing[dim]);
          if(dim == 0)
          {
            for(float32& value : filteredPlane)
            {
              const double derivative = static_cast<double>(value) / derivativeSpacing;
              value = static_cast<float32>(derivative * derivative);
            }
            return acc.copyFromBuffer(planeOffset, nonstd::span<const float32>(filteredPlane.data(), filteredPlane.size()));
          }

          if(Result<> result = acc.copyIntoBuffer(planeOffset, nonstd::span<float32>(accumulatorPlane.data(), filteredPlane.size())); result.invalid())
          {
            return result;
          }
          for(usize i = 0; i < filteredPlane.size(); ++i)
          {
            const double derivative = static_cast<double>(filteredPlane[i]) / derivativeSpacing;
            accumulatorPlane[i] = static_cast<float32>(static_cast<double>(accumulatorPlane[i]) + derivative * derivative);
            if(dim == 1)
            {
              accumulatorPlane[i] = static_cast<float32>(std::sqrt(static_cast<double>(accumulatorPlane[i])));
            }
          }
          return acc.copyFromBuffer(planeOffset, nonstd::span<const float32>(accumulatorPlane.data(), filteredPlane.size()));
        };

        const uint32 otherAxis = dim == 0 ? 1u : 0u;
        const RecursiveGaussianPlaneAxis derivative{dim, sigma, static_cast<double>(spacing[dim]), 1, normalizeAcrossScale};
        const RecursiveGaussianPlaneAxis smoothing{otherAxis, sigma, static_cast<double>(spacing[otherAxis]), 0, normalizeAcrossScale};
        if(Result<> result = RecursiveGaussianXYPlaneCascadeToSink<T>(inStore, dims, derivative, smoothing, shouldCancel, messageHandler, terminalSink); result.invalid())
        {
          return result;
        }
      }
      return {};
    }

    const std::string format = detail::SelectRecursiveGaussianWorkingDataFormat(inStore.getStoreType(), inStore.getDataFormat(), acc.getStoreType(), acc.getDataFormat());
    const std::vector<usize> tupleShape = inArray.getTupleShape();
    auto workPtr = DataStoreUtilities::CreateDataStoreWithFormat<float32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
    AbstractDataStore<float32>& work = *workPtr;

    for(uint32 dim = 0; dim < effDim; ++dim)
    {
      if(shouldCancel)
      {
        return {};
      }
      // order-1 derivative along `dim` (reads raw input T)
      if(Result<> r = RecursiveGaussianAxisPass<T>(inStore, work, dims, dim, sigma, static_cast<double>(spacing[dim]), /*order=*/1, normalizeAcrossScale, shouldCancel, messageHandler); r.invalid())
      {
        return r;
      }
      // order-0 smoothing along every other axis, in increasing order
      for(uint32 a = 0; a < effDim; ++a)
      {
        if(a == dim)
        {
          continue;
        }
        if(Result<> r = RecursiveGaussianAxisPass<float32>(work, work, dims, a, sigma, static_cast<double>(spacing[a]), /*order=*/0, normalizeAcrossScale, shouldCancel, messageHandler); r.invalid())
        {
          return r;
        }
      }
      // dim 0 OVERWRITES acc (no prior fill needed); later dims accumulate. effDim >= 2 always, so dim 0 always runs and
      // fully writes acc. Set-then-Accumulate is bit-identical to the old Fill(0)+Accumulate for every dim.
      if(dim == 0)
      {
        if(Result<> r = SetSquaredOverSpacing(acc, work, static_cast<double>(spacing[dim]), shouldCancel); r.invalid())
        {
          return r;
        }
      }
      else
      {
        if(Result<> r = AccumulateSquaredOverSpacing(acc, work, static_cast<double>(spacing[dim]), shouldCancel); r.invalid())
        {
          return r;
        }
      }
    }
    return SqrtStoreInPlace(acc, shouldCancel);
  }
};

/**
 * @brief Dispatched execute body for the (ITK-free) GradientMagnitude filter. Input is the T-typed (any scalar)
 *        image; output is a FIXED float32 gradient-magnitude image, computed by the single-pass central-difference
 *        streaming engine (@ref ApplyGradientMagnitude) rather than the separable recursive-Gaussian engine used by
 *        @ref GradientMagnitudeRecursiveGaussianExecuteFn above. Matches the legacy ITK GradientMagnitudeImageFilter.
 */
struct GradientMagnitudeExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  FloatVec3 spacing;
  bool useImageSpacing;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputArrayPath);
    return ApplyGradientMagnitude<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, useImageSpacing, spacing, shouldCancel, messageHandler);
  }
};

template <class T>
Result<> ApplyLaplacianRecursiveGaussianResident(const AbstractDataStore<T>& input, AbstractDataStore<float32>& accumulator, AbstractDataStore<float32>& work, const SizeVec3& dims,
                                                 const FloatVec3& spacing, float64 sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel,
                                                 const IFilter::MessageHandler& messageHandler)
{
  const uint32 effectiveDimensions = dims[2] > 1 ? 3u : 2u;
  for(uint32 dim = 0; dim < effectiveDimensions; ++dim)
  {
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = RecursiveGaussianAxisPass<T>(input, work, dims, dim, sigma, static_cast<double>(spacing[dim]), /*order=*/2, normalizeAcrossScale, shouldCancel, messageHandler);
       result.invalid())
    {
      return result;
    }
    for(uint32 axis = 0; axis < effectiveDimensions; ++axis)
    {
      if(axis == dim)
      {
        continue;
      }
      if(Result<> result = RecursiveGaussianAxisPass<float32>(work, work, dims, axis, sigma, static_cast<double>(spacing[axis]), /*order=*/0, normalizeAcrossScale, shouldCancel, messageHandler);
         result.invalid())
      {
        return result;
      }
    }
    const double derivativeSpacing = static_cast<double>(spacing[dim]);
    const double inverseSpacingSquared = 1.0 / (derivativeSpacing * derivativeSpacing);
    if(dim == 0)
    {
      if(Result<> result = SetScaledDerivative(accumulator, work, inverseSpacingSquared, shouldCancel); result.invalid())
      {
        return result;
      }
    }
    else if(Result<> result = AccumulateScaledDerivative(accumulator, work, inverseSpacingSquared, shouldCancel); result.invalid())
    {
      return result;
    }
  }
  return {};
}

/// @brief Dispatched execute body for the LaplacianRecursiveGaussian filter.
/// Input is a T-typed scalar image and output is a fixed float32 Laplacian image. For each filtered axis, it computes the
/// second derivative, smooths along every other axis, and accumulates the spacing-scaled derivative without squaring or
/// a final square root. Actual OOC endpoints use raw fixed-record float32 scratch; forced plane-cascade tests over
/// resident endpoints retain a format-matched DataStore. Output doubles as the accumulator. This preserves ITK's
/// double-coefficient IIR and float32 inter-pass behavior.
struct LaplacianRecursiveGaussianExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  FloatVec3 spacing;
  float64 sigma;
  bool normalizeAcrossScale;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<float32>& acc = outArray.getDataStoreRef(); // output doubles as the accumulator

    const bool usesOutOfCoreStore = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || acc.getStoreType() == IDataStore::StoreType::OutOfCore;
    const bool usePlaneCascade = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
    RecordAlgorithmPathExecution(usePlaneCascade ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

    const uint32 effDim = (dims[2] > 1) ? 3u : 2u;
    if(usesOutOfCoreStore && !ForceInCoreAlgorithm() && detail::ShouldUseLaplacianRecursiveGaussianResidentState(dims))
    {
      auto allocationResult = detail::ReserveLaplacianRecursiveGaussianResidentWorkingMemory<T>(dims);
      if(allocationResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(allocationResult));
      }
      auto allocation = std::move(allocationResult.value());
      if(allocation.holdsCompleteState())
      {
        try
        {
          DataStore<T> residentInput(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, std::nullopt);
          DataStore<float32> residentAccumulator(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, std::nullopt);
          DataStore<float32> residentWork(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, std::nullopt);
          if(Result<> result = inStore.copyIntoBuffer(0, residentInput.createSpan()); result.invalid())
          {
            return result;
          }
          if(Result<> result = ApplyLaplacianRecursiveGaussianResident(residentInput, residentAccumulator, residentWork, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
             result.invalid())
          {
            return result;
          }
          if(shouldCancel)
          {
            return {};
          }
          const auto outputSpan = residentAccumulator.createSpan();
          return acc.copyFromBuffer(0, nonstd::span<const float32>(outputSpan.data(), outputSpan.size()));
        } catch(const std::bad_alloc&)
        {
          // Release the complete-state reservation before entering the raw-record plane-cascade fallback.
        }
      }
    }
    const detail::RecursiveGaussianStoragePlan storagePlan = detail::SelectRecursiveGaussianStoragePlan(inStore.getStoreType(), inStore.getDataFormat(), acc.getStoreType(), acc.getDataFormat());
    if(usePlaneCascade && effDim == 2 && storagePlan.useTemporaryRecordStore)
    {
      detail::RecursiveGaussian2DBufferPlan plan =
          detail::BuildRecursiveGaussian2DBufferPlan(dims[0], dims[1], RecursiveGaussianPassOptions::k_Default2DResidentLimit, std::max<usize>(20, sizeof(T) + 16));
      if(plan.valid && plan.coreCols != dims[0])
      {
        plan = detail::BuildRecursiveGaussian2DBufferPlan(dims[0], dims[1], RecursiveGaussianPassOptions::k_Default2DResidentLimit, std::max<usize>(24, sizeof(T) + 20));
      }
      plan = detail::AlignRecursiveGaussian2DBufferPlanToChunks(plan, dims[0], dims[1], acc.getChunkShape());
      if(plan.overflow || !plan.valid)
      {
        return MakeErrorResult(-23621, fmt::format("Laplacian recursive Gaussian could not build a bounded 2D plan for dimensions {} x {}.", dims[0], dims[1]));
      }
      usize checkpointCount = 0;
      usize workBatchValues = 0;
      if(!detail::RecursiveGaussianCheckedMultiply(plan.blockCount, plan.coreCols, checkpointCount) || !detail::RecursiveGaussianCheckedMultiply(plan.blockRows, plan.coreCols, workBatchValues))
      {
        return MakeErrorResult(-23621, fmt::format("Laplacian recursive Gaussian bounded 2D scratch dimensions overflow for {} x {}.", dims[0], dims[1]));
      }
      checkpointCount = std::max(checkpointCount, detail::RecursiveGaussian2DXBlockCount(dims[0], plan.coreCols));
      auto checkpointResult = detail::CreateGaussianTemporaryStore<detail::RecursiveGaussian2DCheckpoint>(checkpointCount, plan.coreCols, shouldCancel);
      if(checkpointResult.invalid())
      {
        return ConvertResult(std::move(checkpointResult));
      }
      auto workResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), workBatchValues, shouldCancel);
      if(workResult.invalid())
      {
        return ConvertResult(std::move(workResult));
      }
      auto contributionResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), workBatchValues, shouldCancel);
      if(contributionResult.invalid())
      {
        return ConvertResult(std::move(contributionResult));
      }
      std::unique_ptr<detail::GaussianTemporaryStore<detail::RecursiveGaussian2DCheckpoint>> checkpoints = std::move(checkpointResult.value());
      std::unique_ptr<detail::GaussianTemporaryStore<float32>> work = std::move(workResult.value());
      std::unique_ptr<detail::GaussianTemporaryStore<float32>> contribution = std::move(contributionResult.value());
      return ApplyLaplacianRecursiveGaussian2D<T>(inStore, acc, *work, *contribution, *checkpoints, dims, spacing, sigma, normalizeAcrossScale, plan, shouldCancel, messageHandler);
    }
    if(usePlaneCascade && effDim == 3)
    {
      if(storagePlan.useTemporaryRecordStore)
      {
        auto temporaryResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), dims[0] * dims[1], shouldCancel);
        if(temporaryResult.invalid())
        {
          return ConvertResult(std::move(temporaryResult));
        }
        std::unique_ptr<detail::GaussianTemporaryStore<float32>> work = std::move(temporaryResult.value());
        // Raw (non-deflate-backed) full-volume accumulator scratch, created the same way as `work`: every axis
        // cascade's Set/Accumulate below targets this instead of the real deflate-backed output store, so the
        // running Laplacian sum never inflates/deflates against `acc`. Only the finished result is streamed into
        // `acc` once, at the end of ApplyLaplacianRecursiveGaussian3D.
        auto accumulatorResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), dims[0] * dims[1], shouldCancel);
        if(accumulatorResult.invalid())
        {
          return ConvertResult(std::move(accumulatorResult));
        }
        std::unique_ptr<detail::GaussianTemporaryStore<float32>> rawAccumulator = std::move(accumulatorResult.value());
        return ApplyLaplacianRecursiveGaussian3D<T>(inStore, acc, *rawAccumulator, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
      }
      auto work = DataStoreUtilities::CreateDataStoreWithFormat<float32>(storagePlan.workingDataFormat, inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
      return ApplyLaplacianRecursiveGaussian3D<T>(inStore, acc, acc, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
    }
    if(usePlaneCascade)
    {
      std::vector<float32> accumulatorPlane(dims[0] * dims[1]);
      for(uint32 dim = 0; dim < 2; ++dim)
      {
        if(shouldCancel)
        {
          return {};
        }
        const double derivativeSpacing = static_cast<double>(spacing[dim]);
        const double invSpacingSq = 1.0 / (derivativeSpacing * derivativeSpacing);
        auto terminalSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) -> Result<> {
          if(dim == 0)
          {
            for(float32& value : filteredPlane)
            {
              value = static_cast<float32>(static_cast<double>(value) * invSpacingSq);
            }
            return acc.copyFromBuffer(planeOffset, nonstd::span<const float32>(filteredPlane.data(), filteredPlane.size()));
          }

          if(Result<> result = acc.copyIntoBuffer(planeOffset, nonstd::span<float32>(accumulatorPlane.data(), filteredPlane.size())); result.invalid())
          {
            return result;
          }
          for(usize i = 0; i < filteredPlane.size(); ++i)
          {
            accumulatorPlane[i] = static_cast<float32>(static_cast<double>(accumulatorPlane[i]) + static_cast<double>(filteredPlane[i]) * invSpacingSq);
          }
          return acc.copyFromBuffer(planeOffset, nonstd::span<const float32>(accumulatorPlane.data(), filteredPlane.size()));
        };

        const uint32 otherAxis = dim == 0 ? 1u : 0u;
        const RecursiveGaussianPlaneAxis derivative{dim, sigma, static_cast<double>(spacing[dim]), 2, normalizeAcrossScale};
        const RecursiveGaussianPlaneAxis smoothing{otherAxis, sigma, static_cast<double>(spacing[otherAxis]), 0, normalizeAcrossScale};
        if(Result<> result = RecursiveGaussianXYPlaneCascadeToSink<T>(inStore, dims, derivative, smoothing, shouldCancel, messageHandler, terminalSink); result.invalid())
        {
          return result;
        }
      }
      return {};
    }

    const std::string format = detail::SelectRecursiveGaussianWorkingDataFormat(inStore.getStoreType(), inStore.getDataFormat(), acc.getStoreType(), acc.getDataFormat());
    auto work = DataStoreUtilities::CreateDataStoreWithFormat<float32>(format, inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    return ApplyLaplacianRecursiveGaussianResident(inStore, acc, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
  }
};

/// @brief Dispatched execute body for the SmoothingRecursiveGaussian filter.
/// Input and output share the same signed scalar type T. The cascade applies one order-0 pass per active axis in ITK's
/// last-axis-first order. Actual OOC endpoints use raw fixed-record float32 scratch; forced plane-cascade tests over
/// resident endpoints retain a format-matched DataStore. Later passes run float32 in place before the result is cast back
/// to T, preserving ITK's double-coefficient IIR and float32 inter-pass behavior.
struct SmoothingRecursiveGaussianExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  FloatVec3 spacing;
  std::vector<float64> sigma; // per-axis (x,y,z)
  bool normalizeAcrossScale;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    // Output is SameAsInput (type T). Inter-pass values use ITK's float32 InternalRealType. The first pass reads the raw T
    // input; later passes run float32 in place.
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<T>& outStore = outArray.getDataStoreRef();

    const bool usesOutOfCoreStore = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || outStore.getStoreType() == IDataStore::StoreType::OutOfCore;
    const bool usePlaneCascade = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
    RecordAlgorithmPathExecution(usePlaneCascade ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

    const uint32 effDim = (dims[2] > 1) ? 3u : 2u;
    const detail::RecursiveGaussianStoragePlan storagePlan =
        detail::SelectRecursiveGaussianStoragePlan(inStore.getStoreType(), inStore.getDataFormat(), outStore.getStoreType(), outStore.getDataFormat());
    if(usePlaneCascade && effDim == 2 && storagePlan.useTemporaryRecordStore)
    {
      constexpr usize k_BlockBytesPerValue = std::is_same_v<T, float32> ? sizeof(T) + 16 : 2 * sizeof(T) + 16;
      detail::RecursiveGaussian2DBufferPlan plan = detail::BuildRecursiveGaussian2DBufferPlan(dims[0], dims[1], RecursiveGaussianPassOptions::k_Default2DResidentLimit, k_BlockBytesPerValue);
      if(plan.valid && plan.coreCols != dims[0])
      {
        plan = detail::BuildRecursiveGaussian2DBufferPlan(dims[0], dims[1], RecursiveGaussianPassOptions::k_Default2DResidentLimit, k_BlockBytesPerValue + sizeof(float32));
      }
      plan = detail::AlignRecursiveGaussian2DBufferPlanToChunks(plan, dims[0], dims[1], outStore.getChunkShape());
      if(plan.overflow || !plan.valid)
      {
        return MakeErrorResult(-23622, fmt::format("Smoothing recursive Gaussian could not build a bounded 2D plan for dimensions {} x {}.", dims[0], dims[1]));
      }
      usize checkpointCount = 0;
      usize workBatchValues = 0;
      if(!detail::RecursiveGaussianCheckedMultiply(plan.blockCount, plan.coreCols, checkpointCount) || !detail::RecursiveGaussianCheckedMultiply(plan.blockRows, plan.coreCols, workBatchValues))
      {
        return MakeErrorResult(-23622, fmt::format("Smoothing recursive Gaussian bounded 2D checkpoint dimensions overflow for {} x {}.", dims[0], dims[1]));
      }
      checkpointCount = std::max(checkpointCount, detail::RecursiveGaussian2DXBlockCount(dims[0], plan.coreCols));
      auto checkpointResult = detail::CreateGaussianTemporaryStore<detail::RecursiveGaussian2DCheckpoint>(checkpointCount, plan.coreCols, shouldCancel);
      if(checkpointResult.invalid())
      {
        return ConvertResult(std::move(checkpointResult));
      }
      auto workResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), workBatchValues, shouldCancel);
      if(workResult.invalid())
      {
        return ConvertResult(std::move(workResult));
      }
      std::unique_ptr<detail::GaussianTemporaryStore<detail::RecursiveGaussian2DCheckpoint>> checkpoints = std::move(checkpointResult.value());
      std::unique_ptr<detail::GaussianTemporaryStore<float32>> work = std::move(workResult.value());
      return ApplySmoothingRecursiveGaussian2D<T>(inStore, outStore, *work, *checkpoints, dims, spacing, sigma, normalizeAcrossScale, plan, shouldCancel, messageHandler);
    }
    if(usePlaneCascade && effDim == 3)
    {
      if(storagePlan.useTemporaryRecordStore)
      {
        auto temporaryResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), dims[0] * dims[1], shouldCancel);
        if(temporaryResult.invalid())
        {
          return ConvertResult(std::move(temporaryResult));
        }
        std::unique_ptr<detail::GaussianTemporaryStore<float32>> work = std::move(temporaryResult.value());
        return ApplySmoothingRecursiveGaussian3D<T>(inStore, outStore, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
      }
      auto work = DataStoreUtilities::CreateDataStoreWithFormat<float32>(storagePlan.workingDataFormat, inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
      return ApplySmoothingRecursiveGaussian3D<T>(inStore, outStore, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
    }
    if(usePlaneCascade)
    {
      std::vector<T> outputPlane(dims[0] * dims[1]);
      auto outputSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) -> Result<> {
        for(usize i = 0; i < filteredPlane.size(); ++i)
        {
          outputPlane[i] = static_cast<T>(filteredPlane[i]);
        }
        return outStore.copyFromBuffer(planeOffset, nonstd::span<const T>(outputPlane.data(), filteredPlane.size()));
      };

      const RecursiveGaussianPlaneAxis first{1, sigma[1], static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
      const RecursiveGaussianPlaneAxis second{0, sigma[0], static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
      return RecursiveGaussianXYPlaneCascadeToSink<T>(inStore, dims, first, second, shouldCancel, messageHandler, outputSink);
    }

    const std::string format = detail::SelectRecursiveGaussianWorkingDataFormat(inStore.getStoreType(), inStore.getDataFormat(), outStore.getStoreType(), outStore.getDataFormat());
    const std::vector<usize> tupleShape = inArray.getTupleShape();
    auto workPtr = DataStoreUtilities::CreateDataStoreWithFormat<float32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
    AbstractDataStore<float32>& work = *workPtr;

    // Cascade axis order mirrors ITK SmoothingRecursiveGaussian (last axis first, then 0,1,...); order-0 passes commute, so this order only affects float32 rounding, not correctness.
    std::vector<uint32> axisOrder;
    axisOrder.push_back(effDim - 1);
    for(uint32 a = 0; a < effDim - 1; ++a)
    {
      axisOrder.push_back(a);
    }

    bool first = true;
    for(uint32 axis : axisOrder)
    {
      const double s = sigma[axis];
      const double sp = static_cast<double>(spacing[axis]);
      if(first)
      {
        if(Result<> r = RecursiveGaussianAxisPass<T>(inStore, work, dims, axis, s, sp, /*order=*/0, normalizeAcrossScale, shouldCancel, messageHandler); r.invalid())
        {
          return r;
        }
        first = false;
      }
      else
      {
        if(Result<> r = RecursiveGaussianAxisPass<float32>(work, work, dims, axis, s, sp, /*order=*/0, normalizeAcrossScale, shouldCancel, messageHandler); r.invalid())
        {
          return r;
        }
      }
    }
    return CastFloat32StoreTo<T>(work, outStore, shouldCancel);
  }
};

/**
 * @brief Dispatched execute body for the (ITK-free) DiscreteGaussian filter. Input is the T-typed (any scalar) image;
 *        output is SameAsInput (type T). The full separable FIR cascade (verbatim GaussianOperator kernel per axis,
 *        descending axis order, T-typed intermediates with per-pass static_cast<T>, ZeroFluxNeumann boundary) lives in
 *        the streaming engine @ref ApplyDiscreteGaussian, which this functor simply dispatches to. Matches the legacy
 *        ITK DiscreteGaussianImageFilter.
 */
struct DiscreteGaussianExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  FloatVec3 spacing;
  std::vector<float64> variance;     // per-axis (x,y,z)
  std::vector<float64> maximumError; // per-axis (x,y,z)
  uint32 maximumKernelWidth;
  bool useImageSpacing;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    return ApplyDiscreteGaussian<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, variance, maximumKernelWidth, maximumError, useImageSpacing, spacing, shouldCancel, messageHandler);
  }
};

/**
 * @brief Dispatched execute body shared by every ITK-free finite-difference PDE filter (CurvatureFlow now;
 *        MinMaxCurvatureFlow/GradientAnisotropicDiffusion/CurvatureAnisotropicDiffusion later): output is
 *        SameAsInput (type T). The functor @p fn (CurvatureFlowFn etc.) and its extra parameters (StencilRadius,
 *        ConductanceParameter, ...) are supplied by the caller; this functor only resolves the T-typed input/output
 *        stores and forwards to the streaming @ref ApplyFiniteDifference driver.
 */
template <class F>
struct FiniteDifferenceExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  FloatVec3 spacing;
  F fn;
  float64 timeStep;
  uint32 numberOfIterations;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    return ApplyFiniteDifference<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, spacing, fn, timeStep, numberOfIterations, shouldCancel, messageHandler);
  }
};

/**
 * @brief Dispatched execute body for the ApproximateSignedDistanceMap filter -- the ITK composite: IsoContourDistance
 *        (levelSet = (inside+outside)/2, far = maxDist+1) lays a narrow signed band into the float32 output, then
 *        FastChamferDistance refines it IN PLACE (no scratch). maxDist = floor(image diagonal in voxels), z term
 *        excluded for a 2D (nZ==1) image (matching ITK's 2D filter). Output is a FIXED float32 image.
 */
struct ApproximateSignedDistanceMapExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  float64 insideValue;
  float64 outsideValue;
  FloatVec3 spacing;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const int64 nX = static_cast<int64>(dims[0]);
    const int64 nY = static_cast<int64>(dims[1]);
    const int64 nZ = static_cast<int64>(dims[2]);
    const int64 sumSq = nX * nX + nY * nY + ((nZ > 1) ? (nZ * nZ) : 0);
    const float32 maxDist = static_cast<float32>(std::floor(std::sqrt(static_cast<float64>(sumSq))));
    const float64 levelSet = (insideValue + outsideValue) / 2.0;

    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputArrayPath);
    AbstractDataStore<float32>& outStore = outArray.getDataStoreRef();

    if(Result<> r = ApplyIsoContourDistance<T>(inArray.getDataStoreRef(), outStore, dims, levelSet, static_cast<float64>(maxDist) + 1.0, spacing, shouldCancel, messageHandler); r.invalid())
    {
      return r;
    }
    if(Result<> r = ApplyFastChamferDistance(outStore, dims, maxDist, shouldCancel, messageHandler, insideValue > outsideValue); r.invalid())
    {
      return r;
    }
    // IsoContour/chamfer assume "inside" lies below the iso-level. FastChamfer folds the required sign correction
    // into its final writes when InsideValue exceeds OutsideValue, avoiding a separate full-volume OOC pass.
    return {};
  }
};

/**
 * @brief Dispatched execute body for the (ITK-free) ConnectedComponent filter: labels foreground (non-zero) voxels
 *        into consecutive uint32 components via the streaming scanline union-find @ref LabelConnectedComponents
 *        engine. Output is a FIXED uint32 label image, background 0. Input must be an integer scalar type (the
 *        caller's preflight enforces this via the IntegerOnly policy).
 */
struct ConnectedComponentExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  bool fullyConnected;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<uint32>>(outputArrayPath);
    const auto pred = [](T v) { return v != T{}; }; // compile-time predicate (Pred deduced by LabelConnectedComponents)
    return LabelConnectedComponents<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, pred, fullyConnected, shouldCancel, messageHandler);
  }
};

/**
 * @brief Dispatched execute body for the (ITK-free) ZeroCrossing filter: marks each voxel closest to a sign change
 *        (zero crossing) among its axial face neighbors with @c foregroundValue, all others with
 *        @c backgroundValue, via the streamed axial sign-change stencil @ref ApplyZeroCrossing engine. Input must
 *        be a SIGNED scalar type (the caller's preflight enforces this via the SignedScalar policy); output is a
 *        FIXED uint8 label image.
 */
struct ZeroCrossingExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  uint8 foregroundValue;
  uint8 backgroundValue;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outStore = dataStructure.getDataRefAs<DataArray<uint8>>(outputArrayPath).getDataStoreRef();
    return ApplyZeroCrossing<T>(inStore, outStore, dims, foregroundValue, backgroundValue, shouldCancel, messageHandler);
  }
};

/**
 * @brief Dispatched execute body for the (ITK-free) BinaryThinning filter: per-z-slice 2D sequential thinning
 *        (Gonzalez-Woods) via the @ref ApplyBinaryThinning engine, transcribed from itkBinaryThinningImageFilter.hxx.
 *        Foreground (non-zero) voxels are thinned to a 1-pixel skeleton (values 0/1); a 3D image is thinned per-z-slice
 *        independently (ITK's neighbor offsets are 2D). Output is SameAsInput (same type as input). Input must be an
 *        integer scalar type (the caller's preflight enforces this via the IntegerOnly policy).
 */
struct BinaryThinningExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outStore = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath).getDataStoreRef();
    return ApplyBinaryThinning<T>(inStore, outStore, dims, shouldCancel, messageHandler);
  }
};

/**
 * @brief Streamed binary threshold of a scalar input store into a uint8 output store: out[i] =
 *        (lower <= in[i] && in[i] <= upper) ? insideValue : outsideValue. This is the exact ITK
 *        BinaryThresholdImageFilter functor (itkBinaryThresholdImageFilter.h: inclusive on BOTH bounds,
 *        compared in the input element type T). Chunked (bounded-memory) and out-of-core-safe -- it reads/writes
 *        via copyIntoBuffer/copyFromBuffer, so an out-of-core input/scratch store is streamed a chunk at a time.
 *        The DoubleThreshold façade calls this twice (narrow marker band [T2,T3], wide mask band [T1,T4]); the
 *        thresholds are already cast to T by the caller so the comparison is byte-exact vs live ITK.
 */
template <class T>
Result<> StreamBinaryThreshold(const AbstractDataStore<T>& inStore, AbstractDataStore<uint8>& outStore, T lower, T upper, uint8 insideValue, uint8 outsideValue, const std::atomic_bool& shouldCancel)
{
  constexpr usize k_ChunkValues = 65536;
  const usize total = inStore.getSize();
  const usize chunk = std::min<usize>(total, k_ChunkValues);
  std::vector<T> inBuf(chunk);
  std::vector<uint8> outBuf(chunk);
  for(usize offset = 0; offset < total; offset += chunk)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize n = std::min(chunk, total - offset);
    if(Result<> r = inStore.copyIntoBuffer(offset, nonstd::span<T>(inBuf.data(), n)); r.invalid())
    {
      return r;
    }
    for(usize i = 0; i < n; ++i)
    {
      const T v = inBuf[i];
      outBuf[i] = (lower <= v && v <= upper) ? insideValue : outsideValue;
    }
    if(Result<> r = outStore.copyFromBuffer(offset, nonstd::span<const uint8>(outBuf.data(), n)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

template <class T>
Result<> StreamDoubleThresholdMarkers(const AbstractDataStore<T>& inStore, AbstractDataStore<uint8>& markerStore, AbstractDataStore<uint8>& maskStore, T markerLower, T markerUpper, T maskLower,
                                      T maskUpper, uint8 insideValue, uint8 outsideValue, const std::atomic_bool& shouldCancel)
{
  constexpr usize k_ChunkValues = std::max<usize>(1, (16ULL * 1024ULL * 1024ULL) / (sizeof(T) + 2 * sizeof(uint8)));
  const usize total = inStore.getSize();
  const usize chunk = std::min(total, k_ChunkValues);
  auto inputBuffer = std::make_unique<T[]>(chunk);
  auto markerBuffer = std::make_unique<uint8[]>(chunk);
  auto maskBuffer = std::make_unique<uint8[]>(chunk);
  for(usize offset = 0; offset < total; offset += k_ChunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkValues, total - offset);
    if(Result<> r = inStore.copyIntoBuffer(offset, nonstd::span<T>(inputBuffer.get(), count)); r.invalid())
    {
      return r;
    }
    for(usize index = 0; index < count; ++index)
    {
      const T value = inputBuffer[index];
      markerBuffer[index] = (markerLower <= value && value <= markerUpper) ? insideValue : outsideValue;
      maskBuffer[index] = (maskLower <= value && value <= maskUpper) ? insideValue : outsideValue;
    }
    if(Result<> r = markerStore.copyFromBuffer(offset, nonstd::span<const uint8>(markerBuffer.get(), count)); r.invalid())
    {
      return r;
    }
    if(Result<> r = maskStore.copyFromBuffer(offset, nonstd::span<const uint8>(maskBuffer.get(), count)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

/**
 * @brief Dispatched execute body for the (ITK-free) DoubleThreshold filter: a 3-stage mini-pipeline transcribed
 *        from itkDoubleThresholdImageFilter.hxx::GenerateData. (1) marker = BinaryThreshold(input, [T2,T3]); (2)
 *        mask = BinaryThreshold(input, [T1,T4]); (3) output = ReconstructionByDilation(marker, mask,
 *        FullyConnected) run to convergence. The two uint8 scratch stores inherit the input's storage format (an
 *        out-of-core input yields out-of-core scratch), keeping the filter memory-bounded through the streamed
 *        reconstruction sweep. ITK stores the four thresholds as the INPUT pixel type T, so the Float64 params are
 *        cast to T FIRST and the band test is done in T (byte-exact vs live ITK). Output is a FIXED uint8 label
 *        image regardless of the input element type.
 */
struct DoubleThresholdExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  float64 threshold1;
  float64 threshold2;
  float64 threshold3;
  float64 threshold4;
  uint8 insideValue;
  uint8 outsideValue;
  bool fullyConnected;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    auto& outArray = dataStructure.getDataRefAs<DataArray<uint8>>(outputArrayPath);
    AbstractDataStore<uint8>& outStore = outArray.getDataStoreRef();

    // ITK stores m_Threshold1..4 as the INPUT pixel type T -> cast double->T, then compare in T (Parity model).
    // NOTE: the float64->T narrowing is IMPLEMENTATION-DEFINED when the Float64 value falls outside T's range
    // (e.g. the default Threshold3/Threshold4 = 254/255 exceed int8's max of 127). This is INTENTIONAL and matches
    // ITK's identical static_cast<InputPixelType> in itkDoubleThresholdImageFilter -- both this facade and the
    // legacy ITK filter perform the SAME cast on the SAME compiler, so they narrow identically and stay byte-exact.
    // A saturating/clamping cast would DIVERGE from ITK and break the parity gate, so it must stay a plain cast.
    const T t1 = static_cast<T>(threshold1);
    const T t2 = static_cast<T>(threshold2);
    const T t3 = static_cast<T>(threshold3);
    const T t4 = static_cast<T>(threshold4);

    // marker (in [t2,t3]) + mask (in [t1,t4]) uint8 scratch stores, inheriting the input's storage format (OOC-inheriting).
    auto markerStorePtr = DataStoreUtilities::CreateDataStoreWithFormat<uint8>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    auto maskStorePtr = DataStoreUtilities::CreateDataStoreWithFormat<uint8>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    AbstractDataStore<uint8>& markerStore = *markerStorePtr;
    AbstractDataStore<uint8>& maskStore = *maskStorePtr;

    if(Result<> r = StreamDoubleThresholdMarkers<T>(inStore, markerStore, maskStore, t2, t3, t1, t4, insideValue, outsideValue, shouldCancel); r.invalid())
    {
      return r;
    }

    // reconstruction-by-dilation: marker under mask -> out. inArray + outArray supply the OOC storage-path detection
    // (the marker/mask are scratch, not IDataArrays); the data is read from markerStore/maskStore.
    return ApplyMorphologicalReconstruction<uint8>(markerStore, maskStore, outStore, dims, ReconstructOp::Dilation, fullyConnected, inArray, outArray, shouldCancel, messageHandler);
  }
};

/**
 * @brief Dispatched execute body for the (ITK-free) RelabelComponent filter: given an ALREADY-LABELED integer scalar
 *        image, computes each label's pixel count, optionally sorts the labels by size (descending, ties broken by
 *        ASCENDING original label -- transcribed VERBATIM from itk::RelabelComponentImageFilter::GenerateData's
 *        std::sort comparator, since std::sort is not stable and the comparator fully determines the output label
 *        VALUES), drops components smaller than @c minimumObjectSize (remapped to background), and reassigns
 *        consecutive labels 1,2,... in that order. Output is SameAsInput (same type as input). This is a
 *        self-contained streaming size-sort-relabel -- it does NOT use the ConnectedComponent union-find engine (the
 *        input is already labeled). Input must be an integer scalar type (the caller's preflight enforces this via
 *        the IntegerOnly policy).
 */
#ifdef SIMPLNX_ENABLE_MULTICORE
template <class ValueT>
using RelabelThreadLocal = tbb::combinable<ValueT>;
#else
template <class ValueT>
class RelabelThreadLocal
{
public:
  RelabelThreadLocal() = default;

  template <class FactoryT>
  explicit RelabelThreadLocal(FactoryT&& factory)
  : m_Value(std::forward<FactoryT>(factory)())
  {
  }

  ValueT& local()
  {
    return m_Value;
  }

  template <class FunctionT>
  void combine_each(FunctionT&& function) const
  {
    function(m_Value);
  }

private:
  ValueT m_Value{};
};
#endif

inline constexpr int32 k_RelabelComponentInsufficientWorkingMemory = -8591;
inline constexpr int32 k_RelabelComponentResidentStoreMismatch = -8592;

/**
 * @brief Describes a contiguous batch of complete output chunk slabs.
 */
struct RelabelChunkSlabPlan
{
  usize slabValues = 0;
  usize slabBytes = 0;
};

/**
 * @brief Multiplies two sizes when the product fits in `usize`.
 * @param left The first factor.
 * @param right The second factor.
 * @return The product. Returns empty when either factor is zero or the product overflows.
 */
inline std::optional<usize> RelabelCheckedMultiply(usize left, usize right)
{
  if(left == 0 || right == 0 || left > std::numeric_limits<usize>::max() / right)
  {
    return std::nullopt;
  }
  return left * right;
}

/**
 * @brief Creates an output chunk-slab plan for a three-dimensional array.
 * @param tupleShape The output tuple shape in Z, Y, X order.
 * @param outputChunkShape The optional output chunk shape in Z, Y, X order.
 * @param numValues The number of scalar output values.
 * @param valueBytes The size of one scalar output value in bytes.
 * @return A plan for valid shapes. Returns empty for invalid shapes or overflow.
 */
inline std::optional<RelabelChunkSlabPlan> CreateRelabelChunkSlabPlan(const ShapeType& tupleShape, const std::optional<ShapeType>& outputChunkShape, usize numValues, usize valueBytes)
{
  if(tupleShape.size() != 3 || !outputChunkShape.has_value() || outputChunkShape->size() != tupleShape.size() || valueBytes == 0)
  {
    return std::nullopt;
  }

  const ShapeType& chunkShape = *outputChunkShape;
  for(usize dimension = 0; dimension < tupleShape.size(); ++dimension)
  {
    if(tupleShape[dimension] == 0 || chunkShape[dimension] == 0 || chunkShape[dimension] > tupleShape[dimension])
    {
      return std::nullopt;
    }
  }

  const auto planeValues = RelabelCheckedMultiply(tupleShape[1], tupleShape[2]);
  const auto slabValues = planeValues.has_value() ? RelabelCheckedMultiply(chunkShape[0], *planeValues) : std::nullopt;
  const auto tupleCount = planeValues.has_value() ? RelabelCheckedMultiply(tupleShape[0], *planeValues) : std::nullopt;
  const auto slabBytes = slabValues.has_value() ? RelabelCheckedMultiply(*slabValues, valueBytes) : std::nullopt;
  if(!slabValues.has_value() || !tupleCount.has_value() || !slabBytes.has_value() || *tupleCount != numValues)
  {
    return std::nullopt;
  }
  return RelabelChunkSlabPlan{.slabValues = *slabValues, .slabBytes = *slabBytes};
}

/**
 * @brief Selects a bounded staging value count for RelabelComponent.
 *
 * Uses complete output slabs when the grant holds one. Otherwise, uses the largest bounded value count.
 * @param numValues The number of scalar input values.
 * @param grantedBytes The bytes granted for the staging buffer.
 * @param valueBytes The size of one scalar value in bytes.
 * @param slabPlan The optional output chunk-slab plan.
 * @return The number of values that fit in the staging buffer.
 */
inline usize SelectRelabelStagingValues(usize numValues, uint64 grantedBytes, usize valueBytes, const std::optional<RelabelChunkSlabPlan>& slabPlan)
{
  if(numValues == 0 || valueBytes == 0)
  {
    return 0;
  }

  const usize grantedValues = static_cast<usize>(std::min<uint64>(grantedBytes / valueBytes, std::numeric_limits<usize>::max()));
  const usize fallbackValues = std::min(numValues, grantedValues);
  if(!slabPlan.has_value() || grantedBytes < slabPlan->slabBytes)
  {
    return fallbackValues;
  }

  const usize fullSlabs = numValues / slabPlan->slabValues;
  const usize grantSlabs = static_cast<usize>(std::min<uint64>(grantedBytes / slabPlan->slabBytes, std::numeric_limits<usize>::max()));
  const usize stagingSlabs = std::min(fullSlabs, grantSlabs);
  return stagingSlabs == 0 ? fallbackValues : stagingSlabs * slabPlan->slabValues;
}

struct RelabelComponentExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  usize numTuples;
  uint64 minimumObjectSize;
  bool sortByObjectSize;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    auto& outStore = outArray.getDataStoreRef();
    constexpr usize k_TargetChunkBytes = 32 * 1024 * 1024;
    constexpr bool k_UseDenseLabels = sizeof(T) <= sizeof(uint16);
    const auto denseIndex = [](T value) -> usize { return static_cast<usize>(static_cast<int64>(value) - static_cast<int64>(std::numeric_limits<T>::lowest())); };
    const bool bothStoresInMemory = inStore.getStoreType() == IDataStore::StoreType::InMemory && outStore.getStoreType() == IDataStore::StoreType::InMemory;
    const bool usesOutOfCoreStore = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || outStore.getStoreType() == IDataStore::StoreType::OutOfCore;
    const auto* residentInputStore = bothStoresInMemory ? dynamic_cast<const DataStore<T>*>(&inStore) : nullptr;
    auto* residentOutputStore = bothStoresInMemory ? dynamic_cast<DataStore<T>*>(&outStore) : nullptr;
    const bool useResidentSpans = residentInputStore != nullptr && residentOutputStore != nullptr;
    const bool inputByteCountFits = numTuples <= std::numeric_limits<usize>::max() / sizeof(T);
    const usize inputBytes = inputByteCountFits ? numTuples * sizeof(T) : 0;
    const uint64 usefulStagingBytes = inputByteCountFits ? static_cast<uint64>(inputBytes) : std::numeric_limits<uint64>::max();
    auto workingMemoryReservation = usesOutOfCoreStore ? ReserveWorkingMemory(k_TargetChunkBytes, usefulStagingBytes) : CacheMemoryBudgetManager::WorkingMemoryReservation{};
    const uint64 grantedBytes = usesOutOfCoreStore ? workingMemoryReservation.sizeBytes() : k_TargetChunkBytes;
    if(numTuples > 0 && grantedBytes < sizeof(T))
    {
      return MakeErrorResult(k_RelabelComponentInsufficientWorkingMemory, fmt::format("Relabel Component cannot reserve one {}-byte staging value for input array '{}'. Input values: {}; cache "
                                                                                      "budget: {}; maximum working memory: {}; granted bytes: {}. Increase the cache-memory budget.",
                                                                                      sizeof(T), inputArrayPath.toString(), numTuples, CacheMemoryBudgetManager::instance().budgetBytes(),
                                                                                      CacheMemoryBudgetManager::instance().maximumWorkingMemoryBytes(), grantedBytes));
    }
    const auto slabPlan = usesOutOfCoreStore ? CreateRelabelChunkSlabPlan(outArray.getTupleShape(), outStore.getChunkShape(), numTuples, sizeof(T)) : std::nullopt;
    const usize stagingValues = useResidentSpans ? 0 : SelectRelabelStagingValues(numTuples, grantedBytes, sizeof(T), slabPlan);
    if(usesOutOfCoreStore)
    {
      workingMemoryReservation.shrinkTo(static_cast<uint64>(stagingValues) * sizeof(T));
    }
    auto buffer = stagingValues == 0 ? std::unique_ptr<T[]>{} : std::make_unique_for_overwrite<T[]>(stagingValues);
    // Pass 1 counts each non-background label.
    // Narrow labels use a dense table. Wider labels use a sparse table that scales with the number of labels.
    using LabelSizePair = std::pair<T, uint64>;
    std::vector<LabelSizePair> sizeVector;
    {
      if constexpr(k_UseDenseLabels)
      {
        constexpr usize k_DomainSize = usize{1} << (sizeof(T) * 8);
        constexpr usize k_CountLanes = sizeof(T) == 1 ? 8 : 1;
        using DenseCounts = std::array<uint64, k_DomainSize * k_CountLanes>;
        RelabelThreadLocal<DenseCounts> threadCounts([] { return DenseCounts{}; });
        const auto countBuffer = [&](const T* input, usize count) {
          auto countRange = [&](const Range& range) {
            if(shouldCancel)
            {
              return;
            }
            auto& counts = threadCounts.local();
            const auto incrementCount = [&](T value, usize lane) {
              if(value != T{})
              {
                ++counts[denseIndex(value) * k_CountLanes + lane];
              }
            };
            usize i = range.min();
            if constexpr(k_CountLanes == 8)
            {
              for(; i + 8 <= range.max(); i += 8)
              {
#ifndef SIMPLNX_ENABLE_MULTICORE
                if((i % 65536) == 0 && shouldCancel)
                {
                  return;
                }
#endif
                uint64 packedValues = 0;
                std::memcpy(&packedValues, input + i, sizeof(packedValues));
                if(packedValues == 0)
                {
                  continue;
                }
                incrementCount(input[i], 0);
                incrementCount(input[i + 1], 1);
                incrementCount(input[i + 2], 2);
                incrementCount(input[i + 3], 3);
                incrementCount(input[i + 4], 4);
                incrementCount(input[i + 5], 5);
                incrementCount(input[i + 6], 6);
                incrementCount(input[i + 7], 7);
              }
            }
            for(; i < range.max(); ++i)
            {
#ifndef SIMPLNX_ENABLE_MULTICORE
              if((i % 65536) == 0 && shouldCancel)
              {
                return;
              }
#endif
              incrementCount(input[i], i % k_CountLanes);
            }
          };
          ParallelDataAlgorithm parallelAlgorithm;
          parallelAlgorithm.setRange(0, count);
          parallelAlgorithm.execute(countRange);
        };
        if(useResidentSpans)
        {
          const auto inputSpan = residentInputStore->createSpan();
          if(inputSpan.size() != numTuples)
          {
            return MakeErrorResult(k_RelabelComponentResidentStoreMismatch, fmt::format("Relabel Component resident input array '{}' has {} values, but the filter expected {} values.",
                                                                                        inputArrayPath.toString(), inputSpan.size(), numTuples));
          }
          countBuffer(inputSpan.data(), inputSpan.size());
        }
        else
        {
          usize start = 0;
          while(start < numTuples)
          {
            if(shouldCancel)
            {
              return {};
            }
            const usize count = std::min(stagingValues, numTuples - start);
            if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
            {
              return r;
            }
            countBuffer(buffer.get(), count);
            if(shouldCancel)
            {
              return {};
            }
            start += count;
          }
        }
        std::vector<uint64> counts(k_DomainSize, 0);
        threadCounts.combine_each([&](const DenseCounts& localCounts) {
          for(usize index = 0; index < k_DomainSize; ++index)
          {
            for(usize lane = 0; lane < k_CountLanes; ++lane)
            {
              counts[index] += localCounts[index * k_CountLanes + lane];
            }
          }
        });
        if(shouldCancel)
        {
          return {};
        }
        for(usize index = 0; index < counts.size(); ++index)
        {
          if(counts[index] != 0)
          {
            const T label = static_cast<T>(static_cast<int64>(std::numeric_limits<T>::lowest()) + static_cast<int64>(index));
            if(label != T{})
            {
              sizeVector.emplace_back(label, counts[index]);
            }
          }
        }
      }
      else
      {
        std::unordered_map<T, uint64> counts;
        const auto countBuffer = [&](const T* input, usize count) {
          for(usize i = 0; i < count; ++i)
          {
            if((i % 65536) == 0 && shouldCancel)
            {
              return;
            }
            const T value = input[i];
            if(value != T{})
            {
              ++counts[value];
            }
          }
        };
        if(useResidentSpans)
        {
          const auto inputSpan = residentInputStore->createSpan();
          if(inputSpan.size() != numTuples)
          {
            return MakeErrorResult(k_RelabelComponentResidentStoreMismatch, fmt::format("Relabel Component resident input array '{}' has {} values, but the filter expected {} values.",
                                                                                        inputArrayPath.toString(), inputSpan.size(), numTuples));
          }
          countBuffer(inputSpan.data(), inputSpan.size());
        }
        else
        {
          usize start = 0;
          while(start < numTuples)
          {
            if(shouldCancel)
            {
              return {};
            }
            const usize count = std::min(stagingValues, numTuples - start);
            if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
            {
              return r;
            }
            countBuffer(buffer.get(), count);
            if(shouldCancel)
            {
              return {};
            }
            start += count;
          }
        }
        sizeVector.assign(counts.begin(), counts.end());
      }
    }

    if(shouldCancel)
    {
      return {};
    }
    if(sortByObjectSize)
    {
      // Transcribed VERBATIM from itk::RelabelComponentImageFilter::GenerateData's std::sort comparator
      // (itkRelabelComponentImageFilter.hxx): size DESCENDING; ties (equal size) broken by ASCENDING original
      // label. std::sort is NOT stable, so this tie-break -- not "keep relative order" -- is what makes the
      // sorted order (and therefore the output label values) deterministic.
      std::sort(sizeVector.begin(), sizeVector.end(), [](const LabelSizePair& a, const LabelSizePair& b) -> bool { return a.second > b.second || (!(a.second < b.second) && a.first < b.first); });
    }
    else if constexpr(!k_UseDenseLabels)
    {
      // Dense labels were emitted in ascending numeric order. Restore that same ITK m_SizeMap order after the
      // unordered wide-label count path.
      std::sort(sizeVector.begin(), sizeVector.end(), [](const LabelSizePair& a, const LabelSizePair& b) -> bool { return a.first < b.first; });
    }

    if(shouldCancel)
    {
      return {};
    }
    // Build the label -> output-label remap (itkRelabelComponentImageFilter.hxx::GenerateData's relabelMap loop):
    // size < minimumObjectSize -> background (T{}); else the next consecutive label (1, 2, 3, ...).
    std::vector<T> denseRemap;
    std::unordered_map<T, T> sparseRemap;
    if constexpr(k_UseDenseLabels)
    {
      constexpr usize k_DomainSize = usize{1} << (sizeof(T) * 8);
      denseRemap.assign(k_DomainSize, T{});
    }
    else
    {
      sparseRemap.reserve(sizeVector.size());
    }
    T outputLabel{};
    usize survivingObjectCount = 0;
    for(const auto& [label, size] : sizeVector)
    {
      static_cast<void>(label);
      if(minimumObjectSize == 0 || size >= minimumObjectSize)
      {
        ++survivingObjectCount;
      }
    }
    for(const auto& [label, size] : sizeVector)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(minimumObjectSize > 0 && size < minimumObjectSize)
      {
        if constexpr(!k_UseDenseLabels)
        {
          sparseRemap.emplace(label, T{});
        }
      }
      else
      {
        if(outputLabel == std::numeric_limits<T>::max())
        {
          return MakeErrorResult(k_RelabelComponentTooManyObjects,
                                 fmt::format("RelabelComponentImageFilter: too many objects of sufficient size ({}) for the output element type '{}' (max label {}). Reduce the object count or use a "
                                             "wider input/output type.",
                                             survivingObjectCount, DataTypeToString(GetDataType<T>()), static_cast<uint64>(std::numeric_limits<T>::max())));
        }
        outputLabel = static_cast<T>(outputLabel + 1);
        if constexpr(k_UseDenseLabels)
        {
          denseRemap[denseIndex(label)] = outputLabel;
        }
        else
        {
          sparseRemap.emplace(label, outputLabel);
        }
      }
    }

    // Pass 2: resident DataStores copy directly into the final output span, then remap it in place. Other stores
    // stream inStore -> outStore. Each worker touches only its disjoint local-buffer range; abstract DataStore I/O
    // stays serial. Dense remapping is direct-indexed and vectorizable. Wide labels use concurrent const lookups in
    // the immutable sparse table.
    {
      const auto& denseRemapLookup = denseRemap;
      const auto& sparseRemapLookup = sparseRemap;
      const auto remapBuffer = [&](const T* input, T* output, usize count) {
        auto remapRange = [&](const Range& range) {
          if(shouldCancel)
          {
            return;
          }
          usize i = range.min();
          if constexpr(sizeof(T) == 1)
          {
            for(; i + 8 <= range.max(); i += 8)
            {
#ifndef SIMPLNX_ENABLE_MULTICORE
              if((i % 65536) == 0 && shouldCancel)
              {
                return;
              }
#endif
              uint64 packedValues = 0;
              std::memcpy(&packedValues, input + i, sizeof(packedValues));
              if(packedValues == 0)
              {
                if(input != output)
                {
                  std::fill_n(output + i, 8, T{});
                }
                continue;
              }
              output[i] = denseRemapLookup[denseIndex(input[i])];
              output[i + 1] = denseRemapLookup[denseIndex(input[i + 1])];
              output[i + 2] = denseRemapLookup[denseIndex(input[i + 2])];
              output[i + 3] = denseRemapLookup[denseIndex(input[i + 3])];
              output[i + 4] = denseRemapLookup[denseIndex(input[i + 4])];
              output[i + 5] = denseRemapLookup[denseIndex(input[i + 5])];
              output[i + 6] = denseRemapLookup[denseIndex(input[i + 6])];
              output[i + 7] = denseRemapLookup[denseIndex(input[i + 7])];
            }
          }
          for(; i < range.max(); ++i)
          {
#ifndef SIMPLNX_ENABLE_MULTICORE
            if((i % 65536) == 0 && shouldCancel)
            {
              return;
            }
#endif
            const T value = input[i];
            if constexpr(k_UseDenseLabels)
            {
              output[i] = denseRemapLookup[denseIndex(value)];
            }
            else
            {
              output[i] = value == T{} ? T{} : sparseRemapLookup.at(value);
            }
          }
        };
        ParallelDataAlgorithm parallelAlgorithm;
        parallelAlgorithm.setRange(0, count);
        parallelAlgorithm.execute(remapRange);
      };
      if(useResidentSpans)
      {
        const auto inputSpan = residentInputStore->createSpan();
        auto outputSpan = residentOutputStore->createSpan();
        if(inputSpan.size() != numTuples || outputSpan.size() != numTuples)
        {
          return MakeErrorResult(k_RelabelComponentResidentStoreMismatch,
                                 fmt::format("Relabel Component resident stores do not match the expected value count. Input array '{}': {}; output array '{}': {}; expected: {}.",
                                             inputArrayPath.toString(), inputSpan.size(), outputArrayPath.toString(), outputSpan.size(), numTuples));
        }
        if(shouldCancel)
        {
          return {};
        }
        remapBuffer(inputSpan.data(), outputSpan.data(), outputSpan.size());
      }
      else
      {
        for(usize start = 0; start < numTuples;)
        {
          if(shouldCancel)
          {
            return {};
          }
          const usize count = std::min(stagingValues, numTuples - start);
          if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
          {
            return r;
          }
          remapBuffer(buffer.get(), buffer.get(), count);
          if(shouldCancel)
          {
            return {};
          }
          if(Result<> r = outStore.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
          {
            return r;
          }
          start += count;
        }
      }
    }
    return {};
  }
};

/**
 * @brief Clamps the user's Float64 Upper Boundary parameter to the input pixel type T's representable range,
 *        mirroring the legacy ITK bridge functor's SetUpperBoundary call
 *        (ITKThresholdMaximumConnectedComponentsImageFilter.cpp::createFilter): `static_cast<PixelType>(
 *        std::min<MinType>(upperBoundary, itk::NumericTraits<PixelType>::max()))`, where MinType is PixelType
 *        itself for int64/uint64 (whose max cannot be represented exactly as a float64, so the comparison is done
 *        in T to avoid that precision loss) and float64 for every other scalar type (all other integers plus
 *        float32/float64). This is the ONE-TIME clamp applied before the bisection (ITK's `m_UpperBoundary`),
 *        distinct from the per-run LOCAL bisection bound computed in @ref ThresholdMaxCCExecuteFn.
 */
template <class T>
inline T ClampThresholdMaxCCUpperBoundary(float64 upperBoundary)
{
  if constexpr(std::is_same_v<T, int64> || std::is_same_v<T, uint64>)
  {
    return std::min(static_cast<T>(upperBoundary), std::numeric_limits<T>::max());
  }
  else
  {
    return static_cast<T>(std::min(upperBoundary, static_cast<float64>(std::numeric_limits<T>::max())));
  }
}

/**
 * @brief Dispatched execute body for the (ITK-free) ThresholdMaximumConnectedComponents filter: bisection-searches
 *        the threshold value that MAXIMIZES the number of connected components with size >=
 *        @c minimumObjectSizeInPixels, then writes the final binary threshold at the converged value. Transcribed
 *        VERBATIM from itk::ThresholdMaximumConnectedComponentsImageFilter::GenerateData + ComputeConnectedComponents
 *        (itkThresholdMaximumConnectedComponentsImageFilter.hxx):
 *          - lowerBound/upperBound = input min/max (one streaming @ref ComputeArrayMinMax pass); upperBound is
 *            then clamped to the (type-clamped) Upper Boundary parameter -- @ref ClampThresholdMaxCCUpperBoundary.
 *          - midpoint = (upperBound-lowerBound)/2; midpointL = lowerBound+(midpoint-lowerBound)/2; midpointR =
 *            upperBound-(upperBound-midpoint)/2 -- PixelType (T) arithmetic (integer division for integer T, so
 *            this is NOT the true midpoint of [lowerBound,upperBound] when lowerBound != 0); this is ITK's own
 *            formula, transcribed exactly as written, not "fixed".
 *          - while((upperBound-lowerBound) > 2): count components at midpointR and midpointL (each count binary-
 *            thresholds [candidateThreshold, UpperBoundary] and counts connected components with size >=
 *            minimumObjectSizeInPixels, via @ref CountConnectedComponents); if the RIGHT count is STRICTLY greater
 *            than the LEFT count, move lowerBound=midpoint, midpoint=midpointR; otherwise (right <= left, i.e. a
 *            TIE TAKES THE LOWER/LEFT branch, matching ITK's `if (connectedComponentsRight > connectedComponentsLeft)`
 *            exactly) move upperBound=midpoint, midpoint=midpointL. Recompute midpointL/midpointR each iteration.
 *          - final: thresholdValue = midpoint; out = (v >= thresholdValue && v <= UpperBoundary) ? insideValue :
 *            outsideValue.
 *        The internal ConnectedComponentImageFilter connectivity is ITK's own default, FullyConnected=false (the
 *        bridge never calls SetFullyConnected; itkScanlineFilterCommon.h's m_FullyConnected member defaults to
 *        false). Output is a FIXED uint8 binary image. Input may be ANY scalar type (the caller's preflight
 *        enforces this via the AllNumeric policy).
 */
struct ThresholdMaxCCExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  uint32 minimumObjectSizeInPixels;
  float64 upperBoundary;
  uint8 insideValue;
  uint8 outsideValue;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inStore = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath).getDataStoreRef();
    auto& outStore = dataStructure.getDataRefAs<DataArray<uint8>>(outputArrayPath).getDataStoreRef();

    // Input min/max (itk::MinimumMaximumImageCalculator equivalent): one streaming bulk-I/O pass. Only min/max
    // are consumed, so the parallel min/max reduction replaces the full statistics pass.
    Result<ArrayMinMax<T>> statsResult = ComputeArrayMinMax<T>(inStore, shouldCancel);
    if(statsResult.invalid())
    {
      return ConvertResult(std::move(statsResult));
    }
    if(shouldCancel)
    {
      return {};
    }
    const ArrayMinMax<T>& stats = statsResult.value();

    // m_UpperBoundary (ITK): the Upper Boundary parameter clamped to T's representable range -- FIXED for the
    // whole run (unlike the local bisection `upperBound` below, which the loop mutates).
    const T upperBoundaryFixed = ClampThresholdMaxCCUpperBoundary<T>(upperBoundary);

    T lowerBound = stats.min;
    T upperBound = std::min(stats.max, upperBoundaryFixed);

    T midpoint = (upperBound - lowerBound) / 2;
    T midpointL = lowerBound + (midpoint - lowerBound) / 2;
    T midpointR = upperBound - (upperBound - midpoint) / 2;

    const uint64 minSize = static_cast<uint64>(minimumObjectSizeInPixels);
    // ITK's ComputeConnectedComponents (itkThresholdMaximumConnectedComponentsImageFilter.hxx:76-81,108-110) counts
    // components on the BINARY-THRESHOLD OUTPUT, whose foreground is `value != 0`: the internal BinaryThresholdImageFilter
    // writes m_InsideValue to in-band pixels [thr, m_UpperBoundary] and m_OutsideValue elsewhere, then the
    // ConnectedComponentImageFilter (background=0) labels every NONZERO pixel. So a pixel is counted-foreground iff its
    // threshold output would be nonzero -- (in-band ? InsideValue!=0 : OutsideValue!=0) -- NOT simply "in-band". For the
    // DEFAULT (InsideValue=1, OutsideValue=0) this reduces to exactly (in-band ? true : false) = in-band, unchanged; for
    // non-default Inside/Outside (e.g. Inside=0/Outside=1) ITK instead maximizes components in the OUT-of-band region.
    // The internal ConnectedComponentImageFilter connectivity is ITK's default (FullyConnected=false).
    const bool insideNonZero = (insideValue != 0);
    const bool outsideNonZero = (outsideValue != 0);
    auto countAt = [&](T thr) -> Result<uint32> {
      const auto pred = [thr, upperBoundaryFixed, insideNonZero, outsideNonZero](T v) { return (v >= thr && v <= upperBoundaryFixed) ? insideNonZero : outsideNonZero; };
      return CountConnectedComponents<T>(inStore, dims, pred, /*fullyConnected=*/false, minSize, shouldCancel);
    };

    while((upperBound - lowerBound) > T{2})
    {
      if(shouldCancel)
      {
        return {};
      }
      Result<uint32> rightResult = countAt(midpointR);
      if(rightResult.invalid())
      {
        return ConvertResult(std::move(rightResult));
      }
      Result<uint32> leftResult = countAt(midpointL);
      if(leftResult.invalid())
      {
        return ConvertResult(std::move(leftResult));
      }

      // itkThresholdMaximumConnectedComponentsImageFilter.hxx::GenerateData: ties (right <= left) take the ELSE
      // branch -- i.e. the LOWER threshold wins on a tie.
      if(rightResult.value() > leftResult.value())
      {
        lowerBound = midpoint;
        midpoint = midpointR;
      }
      else
      {
        upperBound = midpoint;
        midpoint = midpointL;
      }

      midpointL = lowerBound + (midpoint - lowerBound) / 2;
      midpointR = upperBound - (upperBound - midpoint) / 2;
    }

    const T thresholdValue = midpoint;

    // Final binary threshold [thresholdValue, upperBoundaryFixed] -> insideValue/outsideValue, streamed in a
    // bounded, working-memory-derived band instead of a fixed 64 KiB chunk (identical in-core and out-of-core).
    // A 64 KiB call is smaller than one store chunk at the certified geometry (a Z-plane is exactly one 1 MiB
    // chunk), so every call touched only part of a single chunk and the codec never left its serial branch.
    // Reserving from the shared working-memory budget widens each call to many whole chunks when the grant
    // allows it, and degrades to exactly the previous 64 KiB floor -- never smaller -- under a tight grant.
    const usize total = inStore.getSize();
    constexpr usize k_FinalPassMinimumValues = 65536;
    constexpr usize k_FinalPassPreferredBytes = 32ULL * 1024ULL * 1024ULL;
    constexpr usize k_FinalPassValueBytes = sizeof(T) + sizeof(uint8);
    const bool finalPassByteCountFits = total <= std::numeric_limits<usize>::max() / k_FinalPassValueBytes;
    const uint64 usefulFinalPassBytes = finalPassByteCountFits ? static_cast<uint64>(total) * k_FinalPassValueBytes : std::numeric_limits<uint64>::max();
    auto finalPassReservation = ReserveWorkingMemory(k_FinalPassPreferredBytes, usefulFinalPassBytes);
    const usize chunkValues =
        std::max<usize>(k_FinalPassMinimumValues, std::min(total, static_cast<usize>(std::min<uint64>(finalPassReservation.sizeBytes() / k_FinalPassValueBytes, std::numeric_limits<usize>::max()))));
    // shrinkTo is a no-op when the actual footprint is not smaller than the grant (e.g. the k_FinalPassMinimumValues
    // floor under a tight grant), and otherwise releases the unused portion back to the shared budget immediately,
    // rather than holding it for this call's remaining lifetime.
    finalPassReservation.shrinkTo(static_cast<uint64>(std::min(chunkValues, total)) * k_FinalPassValueBytes);
    auto inBuffer = std::make_unique<T[]>(std::min(chunkValues, total));
    auto outBuffer = std::make_unique<uint8[]>(std::min(chunkValues, total));
    for(usize start = 0; start < total; start += chunkValues)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(chunkValues, total - start);
      if(Result<> r = inStore.copyIntoBuffer(start, nonstd::span<T>(inBuffer.get(), count)); r.invalid())
      {
        return r;
      }
      for(usize i = 0; i < count; ++i)
      {
        const T v = inBuffer[i];
        outBuffer[i] = (v >= thresholdValue && v <= upperBoundaryFixed) ? insideValue : outsideValue;
      }
      if(Result<> r = outStore.copyFromBuffer(start, nonstd::span<const uint8>(outBuffer.get(), count)); r.invalid())
      {
        return r;
      }
    }
    return {};
  }
};
} // namespace detail

/**
 * @brief Execute for a pointwise image filter. Dispatches over the numeric element types via the
 *        core `ExecuteDataFunctionNoBool` helper (no duplicated switch), builds the per-type map
 *        operation from @p operation, and runs the pointwise engine. Single-implementation:
 *        identical code path in-core and OOC.
 *
 * @p operation must expose `template <class T, class U> auto makeMapOp() const` returning a callable
 *   `U(T)`.
 */
template <class TypeSetPolicy = AllNumeric, template <class> class OutTypeMap = SameAsInput, class OperationT>
Result<> ExecuteImageFilter(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const OperationT& operation, const std::atomic_bool& shouldCancel,
                            const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  detail::PointwiseExecuteFn<OutTypeMap, OperationT> executeFn{dataStructure, inputArrayPath, outputArrayPath, operation, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Two-pass execute: computes ArrayStatistics<T> over the input, lets the operation validate the
 *        stats and build a per-pixel map from them, then runs the pointwise engine. Single-implementation
 *        (both passes stream via bulk I/O). @p operation must expose
 *        `template <class T> Result<> validate(const ArrayStatistics<T>&) const` and
 *        `template <class T, class U> auto makeMapOp(const ArrayStatistics<T>&) const`.
 *        An operation
 * that needs only a subset of the statistics may also provide a `computeStatistics<T>` member
 *        returning `Result<ArrayStatistics<T>>`; otherwise the full statistics pass is used.
 */
template <class TypeSetPolicy = AllNumeric, template <class> class OutTypeMap = SameAsInput, class OperationT>
Result<> ExecuteTwoPassImageFilter(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const OperationT& operation, const std::atomic_bool& shouldCancel,
                                   const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  detail::TwoPassExecuteFn<OutTypeMap, OperationT> executeFn{dataStructure, inputArrayPath, outputArrayPath, operation, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute for a two-input mask filter: output[t] = (mask[t] != 0) ? input[t] : outsideValue,
 *        applied to all components of each tuple. Output type == input type. Single-implementation.
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteMaskImageFilter(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& maskArrayPath, const DataPath& outputArrayPath, float64 outsideValue,
                                const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const usize numComponents = inputArray.getNumberOfComponents();
  detail::MaskExecuteFn executeFn{dataStructure, inputArrayPath, maskArrayPath, outputArrayPath, numComponents, outsideValue, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute for a scalar box-neighborhood filter (output type == input type). Dispatches on element
 *        type and runs the box-neighborhood engine with a per-type reduce produced by @p operation
 *        (`template <class T> auto makeReduceOp() const` returning `T(nonstd::span<T>)`).
 */
template <class TypeSetPolicy = AllNumeric, class OperationT>
Result<> ExecuteBoxNeighborhoodImageFilter(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& imageGeomPath, const DataPath& outputArrayPath,
                                           const std::array<usize, 3>& radius, const OperationT& operation, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::BoxNeighborhoodExecuteFn<OperationT> executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, radius, operation, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute for the Adaptive Histogram Equalization filter (Stark 2000). Reads the input dims from the
 *        ImageGeom, dispatches on the input element type via @p TypeSetPolicy (AllNumeric by default -- all 10
 *        scalar types), resolves the in/out stores, and runs the engine (@ref ApplyAdaptiveHistogramEqualization).
 *        Output type == input type (SameAsInput). Single-implementation: identical code path in-core and OOC,
 *        so no DispatchAlgorithm and no in/out IDataArray refs are needed. @p radius is the per-axis box-window
 *        radius; @p alpha / @p beta are the Stark parameters (no range validation -- ITK imposes none).
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteAdaptiveHistogramEqualizationImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                         const std::array<usize, 3>& radius, float32 alpha, float32 beta, const std::atomic_bool& shouldCancel,
                                                         const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::AdaptiveHistogramEqualizationExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, radius, alpha, beta, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute for a flat-structuring-element grayscale morphology filter (Dilate/Erode). Reads the input
 *        dims from the ImageGeom, dispatches on the input element type via @p TypeSetPolicy (AllNumeric by
 *        default), resolves the in/out stores, and runs the morphology engine (@ref ApplyMorphology, which
 *        internally routes the in-core moving-histogram Direct path vs the out-of-core Scanline path via
 *        DispatchAlgorithm). Output type == input type (SameAsInput). @p se is the rasterized structuring
 *        element (built once by the caller from the kernel type + radius); @p op selects Dilate (max fold)
 *        or Erode (min fold). Single façade shared by every morphology filter (Task 6 composites reuse it).
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteMorphologyImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const StructuringElement& se,
                                      MorphOp op, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::MorphologyExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, se, op, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute for a flat-structuring-element OBJECT morphology filter (Dilate/Erode Object Morphology).
 *        Object morphology is a boundary SCATTER, distinct from the grayscale/binary SE min/max gather: only
 *        the boundary voxels of the object paint the structuring element around themselves into a separate
 *        output (see @ref ApplyObjectMorphology). Reads the input dims from the ImageGeom, builds the fixed
 *        radius-1 full-box boundary neighborhood once, dispatches on the input element type via @p TypeSetPolicy
 *        (AllNumeric by default -- object morphology accepts all 10 scalar types), resolves the in/out stores,
 *        and runs the engine (which routes the ITK-faithful in-core Scatter path vs the OOC streamed Gather path
 *        via DispatchAlgorithm). Output type == input type (SameAsInput).
 *
 * @p se is the paint structuring element (built once by the caller from the kernel type + radius); @p op selects
 * Dilate (paint the object value) or Erode (paint the background value). @p objectValue / @p backgroundValue are
 * the Float64 parameter values cast to the element type inside the dispatched functor; the caller MUST have
 * validated them against the element type at preflight (see @ref ValidateScalarValueForType) so the narrowing
 * conversion is well-defined. Single façade shared by both object-morphology filters.
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteObjectMorphologyImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const StructuringElement& se,
                                            ObjectMorphOp op, float64 objectValue, float64 backgroundValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const std::vector<SEOffset> boxOffsets = detail::MakeFullBoxNeighborOffsets();
  detail::ObjectMorphologyExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, se, boxOffsets, op, objectValue, backgroundValue, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute for a radius-1 local-neighbor CONTOUR filter (e.g. Binary Contour). Reads the input dims from
 *        the ImageGeom, builds the connectivity neighbor offsets from @p fullyConnected (Cross for face /
 *        Box for full connectivity, center excluded; see @ref MakeContourNeighborOffsets), dispatches on the
 *        input element type via @p TypeSetPolicy (IntegerOnly by default -- contour is defined on integer label
 *        images), resolves the in/out stores, and runs the contour engine (@ref ApplyContour). Output type ==
 *        input type (SameAsInput). Single-implementation: the same Z-slab streaming path in-core and OOC.
 *
 * @p predicateFactory is a small factory object exposing `template <class T> auto make() const` that returns
 * the per-element-type contour predicate (e.g. @ref BinaryContourPredicateFactory). Passing a factory rather
 * than a bare predicate lets the façade dispatch over every integer element type while keeping the per-voxel
 * predicate typed (the same idiom the pointwise/box-neighborhood façades use with their `operation` object).
 * The factory casts any Float64 foreground/background parameter to the integer element type; the caller MUST
 * have validated those against the element type's range at preflight (see @ref ValidateBinaryFgBgInRange) so
 * the narrowing conversion is well-defined. A later Label contour filter reuses this façade with its own
 * factory. Unlike the binary morphology façade there is NO boundary handling and NO binary-input safeguard:
 * the contour predicate passes any non-foreground voxel through unchanged, so it is ITK-faithful on any input.
 */
template <class TypeSetPolicy = IntegerOnly, class PredicateFactoryT>
Result<> ExecuteContourImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, bool fullyConnected,
                                   const PredicateFactoryT& predicateFactory, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const std::vector<SEOffset> neighborOffsets = MakeContourNeighborOffsets(fullyConnected);
  detail::ContourExecuteFn<PredicateFactoryT> executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, neighborOffsets, predicateFactory, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute for a flat-structuring-element BINARY morphology filter (Binary Dilate/Erode). The binary
 *        analogue of @ref ExecuteMorphologyImageFilter: reads the input dims from the ImageGeom, dispatches on
 *        the input element type via @p TypeSetPolicy (IntegerOnly by default -- binary morphology is defined on
 *        integer label images), resolves the in/out stores, and runs the binary morphology engine
 *        (@ref ApplyBinaryMorphology, which internally routes the in-core moving fg-count Direct path vs the
 *        out-of-core Scanline path via DispatchAlgorithm). Output type == input type (SameAsInput).
 *
 * @p se is the rasterized structuring element (built once by the caller from the kernel type + radius); @p op
 * selects Dilate (any foreground neighbor -> foreground) or Erode (all foreground neighbors -> foreground).
 * Membership is equality-based: a voxel is foreground iff it equals @p foreground, so an arbitrary-label input
 * is handled correctly and the output is strictly {foreground, background}. @p boundaryToForeground controls
 * whether an out-of-image neighbor counts as foreground. @p foreground / @p background are passed as Float64
 * (the parameter type) and cast to the integer element type inside the dispatched functor; the caller MUST
 * have validated them against the element type's range at preflight (see the Binary Dilate/Erode filters'
 * fg/bg guard) so the narrowing conversion is well-defined. Single façade shared by both binary morphology
 * filters (and reusable by later binary composites).
 */
template <class TypeSetPolicy = IntegerOnly>
Result<> ExecuteBinaryMorphologyImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const StructuringElement& se,
                                            MorphOp op, float64 foreground, float64 background, bool boundaryToForeground, const std::atomic_bool& shouldCancel,
                                            const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::BinaryMorphologyExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, se, op, foreground, background, boundaryToForeground, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/// @brief Executes a composite grayscale-morphology filter.
/// @details Dispatches the input element type through @p TypeSetPolicy.
/// The output element type equals the input element type.
/// Each pass selects the in-core direct engine or OOC scanline engine.
/// Scratch stores inherit the input format, so OOC passes remain streamed.
/// Without SafeBorder, composites use one original-sized scratch store.
/// With SafeBorder, composites use two padded scratch stores.
/// Padding uses @ref SafeBorderPadRadius; cropping follows both passes.
/// Thus peak scratch is one original-sized store or two padded stores.
/// Morphological gradient ignores @p safeBorder and uses no scratch store.
/// Unpadded top hats finalize with @ref SubtractStores.
/// SafeBorder top hats fuse crop and subtraction with @ref CropSubtractFromStore.
/// SafeBorder opening and closing only crop the final padded result.
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteMorphologyCompositeImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                               const StructuringElement& se, MorphCompositeOp op, bool safeBorder, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::MorphologyCompositeExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, se, op, safeBorder, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute for a COMPOSITE BINARY-morphology filter (Binary Morphological Opening/Closing). The binary
 *        analogue of @ref ExecuteMorphologyCompositeImageFilter: reads the input dims from the ImageGeom,
 *        dispatches on the input element type via @p TypeSetPolicy (IntegerOnly by default -- binary morphology
 *        is defined on integer label images), runs the binary-input safeguard EXACTLY ONCE on the original
 *        input, then composes the two-pass sequence for @p op through @ref ApplyBinaryMorphology (which routes
 *        the in-core moving fg-count Direct path vs the OOC Scanline path per pass). Output type == input type
 *        (SameAsInput). @p se is the rasterized structuring element (built once by the caller). Scratch stores
 *        are allocated per run inheriting the active OOC endpoint format, so an OOC endpoint keeps the composite
 *        bounded-memory and streamed: Opening and the plain (SafeBorder == false) Closing use ONE same-extent
 *        scratch, while Closing's SafeBorder == true path allocates TWO ping-pong scratches sized to the volume
 *        padded by the kernel radius.
 *
 * @p foreground / @p background are the Float64 parameter values (the caller MUST have range-validated them at
 * preflight so the narrowing cast to the integer element type is well-defined). @p background is used only by
 * Opening; Closing derives its own internal background (0, or the type max iff foreground is 0). @p safeBorder
 * selects Closing's boundary convention (ignored by Opening, which never pads): true (the ITK default) pads by
 * the kernel radius with the internal background, runs both passes on the padded volume, and crops back --
 * bit-identical to the legacy ITK filter; false runs the plain two-pass composition on the original extent.
 */
template <class TypeSetPolicy = IntegerOnly>
Result<> ExecuteBinaryMorphologyCompositeImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                     const StructuringElement& se, BinaryCompositeOp op, float64 foreground, float64 background, bool safeBorder, const std::atomic_bool& shouldCancel,
                                                     const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::BinaryMorphologyCompositeExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, se, op, foreground, background, safeBorder, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/// @brief Executes a histogram-driven classification filter such as multi-Otsu thresholding.
/// @details Integral inputs fuse range and exact-frequency discovery into one bounded scan when their observed span
/// is at most 65,535. Wider integral ranges use the completed range plus a second histogram scan. Floating-point
/// inputs retain separate streaming statistics and histogram scans. Derived thresholds feed the pointwise engine.
/// The same bounded implementation serves in-core and out-of-core stores.
/// @tparam OutTypeMap Maps each input type to the output type. The default creates a uint8 label image.
/// @tparam OperationT Exposes the histogram and threshold settings plus a typed classification-operation factory.
template <class TypeSetPolicy = AllNumeric, template <class> class OutTypeMap = AlwaysUInt8, class OperationT>
Result<> ExecuteHistogramImageFilter(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const OperationT& operation, const std::atomic_bool& shouldCancel,
                                     const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  detail::HistogramExecuteFn<OutTypeMap, OperationT> executeFn{dataStructure, inputArrayPath, outputArrayPath, operation, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute for an axis-projection filter. Reads the input dims from the ImageGeom, resolves the
 *        input/output stores, dispatches on the input element type via @p TypeSetPolicy (ProjectionScalar's
 *        4 types uint8/int16/uint16/float32 by default; Binary passes AllNumeric), and runs the
 *        axis-projection engine with the supplied reduce functor. The output element type is the input type
 *        mapped through @p OutTypeMap (SameAsInput for Max/Min/Median/Binary; a fixed float map for
 *        Mean/StdDev/Sum) -- this MUST be the same OutTypeMap passed to PreflightAxisProjection, or the
 *        resolved output store type will not match what was allocated. @p TypeSetPolicy MUST likewise match
 *        the one PreflightAxisProjection validated against, so the dispatched-over types and the accepted
 *        types agree. Single-implementation: identical code path in-core and OOC.
 *
 * @p outputArrayPath is the array's location DURING execute; for an in-place run that is the temporary
 *   geometry created by PreflightAxisProjection (see ProjectionOutputArrayPath), which the deferred swap
 *   renames to the original geometry afterward. @p imageGeomPath is always the ORIGINAL geometry (still
 *   intact during executeImpl), so the full input dims are read from it.
 */
template <class TypeSetPolicy = ProjectionScalar, template <class> class OutTypeMap = SameAsInput, class ReduceFn>
Result<> ExecuteAxisProjectionImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, usize projDim,
                                          const ReduceFn& reduce, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::AxisProjectionExecuteFn<OutTypeMap, ReduceFn> executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, projDim, reduce, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute a grayscale morphological reconstruction filter (GrayscaleFillhole / GrayscaleGrindPeak / HMaxima
 *        / HMinima / HConvex). Reads the volume dims from @p imageGeomPath, derives the marker selected by
 *        @p markerKind from the single input array, and reconstructs it under the input (mask) via the D3-split
 *        engine (in-core Vincent hybrid vs streamed out-of-core sweep, dispatched on the input/output storage
 *        mode). @p op / @p fullyConnected pick direction + connectivity; @p height feeds the HMaxima/HMinima
 *        markers (ignored by the border markers). When @p differenceFromInput is true (HConvex: the h-dome), the
 *        reconstruction result is subtracted from the input in place after reconstruction (output = input -
 *        reconstruction). Output type == input type (SameAsInput). The caller's preflight must have enforced a
 *        scalar input (@ref PreflightImageFilter requireScalar=true) and, for the height markers, a finite
 *        @p height.
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteMorphologicalReconstructionImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, ReconstructOp op,
                                                       bool fullyConnected, detail::ReconMarker markerKind, float64 height, const std::atomic_bool& shouldCancel,
                                                       const IFilter::MessageHandler& messageHandler, bool differenceFromInput = false)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::MorphologicalReconstructionExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath,    dims, op, fullyConnected, markerKind, height,
                                                         shouldCancel,  messageHandler, differenceFromInput};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute opening-by-reconstruction (@p firstOp == Erode) or closing-by-reconstruction (@p firstOp ==
 *        Dilate): a grayscale morphological erode/dilate with @p se followed by a geodesic reconstruction of that
 *        result under the input, optionally re-valued to preserve original intensities (@p preserveIntensities).
 *        Output type == input type (SameAsInput). The caller's preflight must have enforced a scalar input.
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteReconstructionOpenCloseImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                   const StructuringElement& se, MorphOp firstOp, bool fullyConnected, bool preserveIntensities, const std::atomic_bool& shouldCancel,
                                                   const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::ReconstructionOpenCloseExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, se, firstOp, fullyConnected, preserveIntensities, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute binary opening by reconstruction (integer element types): a binary erosion with @p se followed by
 *        a binary reconstruction-by-dilation retaining the foreground (@p foreground) components of the input that
 *        survive the erosion; all else @p background. Output type == input type. The caller's preflight must have
 *        enforced a scalar integer input and a valid fg/bg range.
 */
template <class TypeSetPolicy = IntegerOnly>
Result<> ExecuteBinaryOpeningByReconstructionImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                         const StructuringElement& se, float64 foreground, float64 background, bool fullyConnected, const std::atomic_bool& shouldCancel,
                                                         const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::BinaryReconstructionOpeningExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, se, foreground, background, fullyConnected, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute a valued regional-extrema filter (ValuedRegionalMaxima if @p op == Maxima, ValuedRegionalMinima if
 *        Minima): pixels belonging to a regional extremum keep their value; every other pixel is set to the marker
 *        value (type lowest for maxima, type max for minima). Routes the in-core flood vs the out-of-core sweep via
 *        the D3-split engine. Output type == input type. The caller's preflight must have enforced a scalar input.
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteValuedRegionalExtremaImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, RegionalExtremaOp op,
                                                 bool fullyConnected, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::ValuedRegionalExtremaExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, op, fullyConnected, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute a binary-output regional-extrema filter (RegionalMaxima if @p op == Maxima, RegionalMinima if
 *        Minima): the regional extrema become @p foreground and everything else @p background. A flat image becomes
 *        all foreground (if @p flatIsExtremum) or all background. The output is a fixed uint32 label image (matching
 *        the legacy ITK FilterOutputType), not the input element type. The caller's preflight must have enforced a
 *        scalar input and a valid @p foreground / @p background for the element type.
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteRegionalExtremaImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, RegionalExtremaOp op,
                                           bool fullyConnected, float64 foreground, float64 background, bool flatIsExtremum, const std::atomic_bool& shouldCancel,
                                           const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::RegionalExtremaExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, op, fullyConnected, foreground, background, flatIsExtremum, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the Signed Maurer Distance Map filter: computes the exact signed Euclidean (or squared-Euclidean)
 *        distance transform of the object defined by the input image, where a voxel is "inside" the object iff its
 *        value is not @p backgroundValue. Distances are negative inside and positive outside the object (or the
 *        reverse when @p insideIsPositive), optionally scaled by the voxel spacing (@p useSpacing) and optionally
 *        left as squared distances (@p squaredDistance). The output is a FIXED float32 distance image (matching the
 *        legacy ITK FilterOutputType), not the input element type. The caller's preflight must have enforced a
 *        scalar integer input.
 */
template <class TypeSetPolicy = IntegerOnly>
Result<> ExecuteSignedMaurerDistanceMapImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, bool insideIsPositive,
                                                   bool squaredDistance, bool useSpacing, float64 backgroundValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::SignedMaurerDistanceMapExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims,         insideIsPositive, squaredDistance,
                                                     useSpacing,    spacing,        backgroundValue, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the (unsigned) Danielsson Distance Map filter: computes the Danielsson 4SED approximate Euclidean (or
 *        squared-Euclidean) distance from each background (zero) voxel to the nearest foreground (nonzero) voxel,
 *        optionally scaled by the voxel spacing (@p useSpacing). The output is a FIXED float32 distance image
 *        (matching the legacy ITK FilterOutputType), not the input element type. @p inputIsBinary is carried for
 *        interface/SIMPL parity but does not affect the distance output. The caller's preflight must have enforced a
 *        scalar integer input.
 */
template <class TypeSetPolicy = IntegerOnly>
Result<> ExecuteDanielssonDistanceMapImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, bool inputIsBinary,
                                                 bool squaredDistance, bool useSpacing, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::DanielssonDistanceMapExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, inputIsBinary, squaredDistance, useSpacing, spacing, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the Signed Danielsson Distance Map filter: the ITK composite of two Danielsson passes (on the input
 *        and on the dilated inverted input) subtracted to produce a SIGNED distance -- negative inside the object and
 *        positive outside (or the reverse when @p insideIsPositive), optionally squared (@p squaredDistance) and
 *        optionally spacing-weighted (@p useSpacing). The output is a FIXED float32 distance image. The caller's
 *        preflight must have enforced a scalar integer input.
 */
template <class TypeSetPolicy = IntegerOnly>
Result<> ExecuteSignedDanielssonDistanceMapImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                       bool insideIsPositive, bool squaredDistance, bool useSpacing, const std::atomic_bool& shouldCancel,
                                                       const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::SignedDanielssonDistanceMapExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, insideIsPositive, squaredDistance, useSpacing, spacing, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the IsoContourDistance filter: a narrow-band SIGNED distance around the @p levelSetValue iso-contour
 *        of the input, with far voxels set to +-@p farValue. The output is a FIXED float32 image, not the input
 *        element type. Input may be ANY scalar type (integer OR floating point). The caller's preflight must have
 *        enforced a scalar input.
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteIsoContourDistanceImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, float64 levelSetValue,
                                              float64 farValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::IsoContourDistanceExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, levelSetValue, farValue, spacing, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the GradientMagnitudeRecursiveGaussian filter: sqrt(sum over filtered axes of (d/dx_i of the
 *        Gaussian-smoothed image)^2), computed with the separable Deriche recursive-Gaussian engine. Sigma is a single
 *        scalar shared by every axis. The output is a FIXED float32 image, not the input element type. Input may be ANY
 *        scalar type (integer OR floating point). The caller's preflight must have enforced a scalar input and the
 *        >= 4-pixels-per-axis dimension requirement.
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteGradientMagnitudeRecursiveGaussianImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                              float64 sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::GradientMagnitudeRecursiveGaussianExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the (ITK-free) GradientMagnitude filter: a single-pass central-difference (order-1, [0.5, 0, -0.5])
 *        gradient magnitude, computed by the streaming @ref ApplyGradientMagnitude engine rather than the separable
 *        recursive-Gaussian engine used by @ref ExecuteGradientMagnitudeRecursiveGaussianImageFilter above. When @p
 *        useImageSpacing is set, each axis's tap is scaled by 1/spacing[axis] (physical-space derivative); otherwise
 *        the derivative is taken in isotropic voxel space. The output is a FIXED float32 image, not the input element
 *        type. Input may be ANY scalar type (integer OR floating point). The caller's preflight must have enforced a
 *        scalar input; there is no minimum-dimension requirement (unlike the recursive-Gaussian family).
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteGradientMagnitudeImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, bool useImageSpacing,
                                             const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::GradientMagnitudeExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, spacing, useImageSpacing, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the LaplacianRecursiveGaussian filter: the sum over filtered axes of the second derivative
 *        (d^2/dx_i^2 of the Gaussian-smoothed image), computed with the separable Deriche recursive-Gaussian engine.
 *        Sigma is a single scalar shared by every axis. The output is a FIXED float32 image, not the input element
 *        type. Input may be ANY scalar type (integer OR floating point). Unlike GradientMagnitude there is no squaring
 *        and no final sqrt. The caller's preflight must have enforced a scalar input and the >= 4-pixels-per-axis
 *        dimension requirement.
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteLaplacianRecursiveGaussianImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, float64 sigma,
                                                      bool normalizeAcrossScale, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::LaplacianRecursiveGaussianExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the SmoothingRecursiveGaussian filter: a separable Gaussian smoothing (per-axis sigma) computed with
 *        the Deriche recursive-Gaussian engine. The output has the SAME type as the input (type-preserving), so this
 *        uses the SignedScalar policy (signed integer + floating point; unsigned excluded because the Deriche IIR has a
 *        small negative overshoot that would underflow an unsigned round-trip). The caller's preflight must have
 *        enforced a scalar input and the >= 4-pixels-per-axis dimension requirement.
 */
template <class TypeSetPolicy = SignedScalar>
Result<> ExecuteSmoothingRecursiveGaussianImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                      const std::vector<float64>& sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::SmoothingRecursiveGaussianExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the (ITK-free) DiscreteGaussian filter: a separable FIR discrete-Gaussian convolution (verbatim
 *        GaussianOperator modified-Bessel kernel per axis, descending axis order, ZeroFluxNeumann boundary), computed
 *        by the streaming @ref ApplyDiscreteGaussian engine. The output has the SAME type as the input, so intermediate
 *        passes are T-typed and integer types truncate between passes (matching ITK's actual behavior). When @p
 *        useImageSpacing is set, each axis's variance is divided by spacing[axis]^2 before the kernel is built. Input
 *        may be ANY scalar type (integer OR floating point); the caller's preflight must have enforced a scalar input.
 *        There is no minimum-dimension requirement.
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteDiscreteGaussianImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                            const std::vector<float64>& variance, uint32 maximumKernelWidth, const std::vector<float64>& maximumError, bool useImageSpacing,
                                            const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::DiscreteGaussianExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, spacing, variance, maximumError, maximumKernelWidth, useImageSpacing, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute a finite-difference PDE image filter (CurvatureFlow now; MinMaxCurvatureFlow/Gradient- and
 *        Curvature-AnisotropicDiffusion later) via the shared @ref ApplyFiniteDifference streaming driver. The
 *        output has the SAME type as the input (SameAsInput). @p fn carries the per-filter update functor
 *        (CurvatureFlowFn etc.), which may itself hold extra parameters set by the caller. @p TypeSetPolicy
 *        selects which scalar types the filter accepts (AllNumeric for CurvatureFlow; FloatingScalar for the
 *        other three). The caller's preflight must have enforced a scalar input.
 */
template <class F, class TypeSetPolicy>
Result<> ExecuteFiniteDifferenceImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, F fn, float64 timeStep,
                                            uint32 numberOfIterations, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::FiniteDifferenceExecuteFn<F> executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, spacing, fn, timeStep, numberOfIterations, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the ApproximateSignedDistanceMap filter: the ITK composite (IsoContourDistance with
 *        levelSet=(InsideValue+OutsideValue)/2 then FastChamferDistance) producing an approximate SIGNED distance to
 *        the boundary between the InsideValue and OutsideValue regions of an integer mask. Output is a FIXED float32
 *        image. The caller's preflight must have enforced a scalar integer input.
 */
template <class TypeSetPolicy = IntegerOnly>
Result<> ExecuteApproximateSignedDistanceMapImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                        float64 insideValue, float64 outsideValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::ApproximateSignedDistanceMapExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, insideValue, outsideValue, spacing, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the (ITK-free) ConnectedComponent filter: labels foreground (non-zero) voxels of an integer scalar
 *        image into consecutive uint32 components (background 0), via the streaming scanline union-find @ref
 *        LabelConnectedComponents engine. The output is a FIXED uint32 label image, not the input element type.
 *        Reproduces legacy ITK's ConnectedComponentImageFilter label VALUES exactly (see the Parity model). The
 *        caller's preflight must have enforced an integer scalar input.
 */
template <class TypeSetPolicy = IntegerOnly>
Result<> ExecuteConnectedComponentImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, bool fullyConnected,
                                              const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::ConnectedComponentExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, imageGeom.getDimensions(), fullyConnected, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the (ITK-free) ZeroCrossing filter: marks the voxels closest to sign changes (zero crossings) among
 *        their axial face neighbors with @p foregroundValue and all others with @p backgroundValue, via the streamed
 *        axial sign-change stencil @ref ApplyZeroCrossing engine. The output is a FIXED uint8 label image, not the
 *        input element type. Reproduces legacy ITK's ZeroCrossingImageFilter output VALUES exactly (see the Parity
 *        model). The caller's preflight must have enforced a signed scalar input.
 */
template <class TypeSetPolicy = SignedScalar>
Result<> ExecuteZeroCrossingImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, uint8 foregroundValue,
                                        uint8 backgroundValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::ZeroCrossingExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, imageGeom.getDimensions(), foregroundValue, backgroundValue, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the (ITK-free) DoubleThreshold filter: thresholds the input into a narrow marker band [T2,T3] and
 *        a wide mask band [T1,T4] (two streamed BinaryThresholds into uint8 scratch), then reconstructs the marker
 *        under the mask by dilation to convergence (@ref ApplyMorphologicalReconstruction). The output is a FIXED
 *        uint8 label image, not the input element type. Reproduces legacy ITK's DoubleThresholdImageFilter output
 *        VALUES exactly (see the Parity model): the thresholds are cast to the input element type T before the band
 *        test, matching ITK's InputPixelType-typed thresholds. The caller's preflight must have enforced a scalar
 *        input.
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteDoubleThresholdImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, float64 threshold1,
                                           float64 threshold2, float64 threshold3, float64 threshold4, uint8 insideValue, uint8 outsideValue, bool fullyConnected, const std::atomic_bool& shouldCancel,
                                           const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::DoubleThresholdExecuteFn executeFn{dataStructure,  inputArrayPath, outputArrayPath, imageGeom.getDimensions(), threshold1, threshold2, threshold3, threshold4, insideValue, outsideValue,
                                             fullyConnected, shouldCancel,   messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the (ITK-free) RelabelComponent filter: given an ALREADY-LABELED integer scalar image, computes
 *        each label's pixel count, optionally sorts by size (descending, ties by ascending original label -- see
 *        @ref detail::RelabelComponentExecuteFn), drops components smaller than @p minimumObjectSize (mapped to
 *        background), and reassigns consecutive labels 1,2,... in that order. Output is SameAsInput -- this does
 *        NOT run the ConnectedComponent union-find engine. Reproduces legacy ITK's RelabelComponentImageFilter
 *        label VALUES exactly (see the Parity model). The caller's preflight must have enforced an integer scalar
 *        input.
 */
template <class TypeSetPolicy = IntegerOnly>
Result<> ExecuteRelabelComponentImageFilter(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& outputArrayPath, uint64 minimumObjectSize, bool sortByObjectSize,
                                            const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  detail::RelabelComponentExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, inputArray.getNumberOfTuples(), minimumObjectSize, sortByObjectSize, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the (ITK-free) BinaryThinning filter: thins the foreground (non-zero) voxels of an integer scalar
 *        image to a 1-pixel-wide skeleton (values 0/1) via the per-z-slice 2D sequential thinning @ref
 *        ApplyBinaryThinning engine. A 3D image is thinned per-z-slice independently (ITK's neighbor offsets are 2D),
 *        so the output is SameAsInput (same type as input). Reproduces legacy ITK's BinaryThinningImageFilter output
 *        VALUES exactly (see the Parity model). The caller's preflight must have enforced an integer scalar input.
 */
template <class TypeSetPolicy = IntegerOnly>
Result<> ExecuteBinaryThinningImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                          const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::BinaryThinningExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, imageGeom.getDimensions(), shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the (ITK-free) ThresholdMaximumConnectedComponents filter: bisection-searches the threshold value
 *        that MAXIMIZES the number of connected components (size >= MinimumObjectSizeInPixels) via @ref
 *        detail::ThresholdMaxCCExecuteFn (which uses @ref CountConnectedComponents each iteration), then writes the
 *        final binary threshold [ThresholdValue, UpperBoundary] as a FIXED uint8 image (InsideValue/OutsideValue).
 *        Reproduces legacy ITK's ThresholdMaximumConnectedComponentsImageFilter output exactly (see the Parity
 *        model). The caller's preflight must have enforced a scalar input.
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteThresholdMaximumConnectedComponentsImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                               uint32 minimumObjectSizeInPixels, float64 upperBoundary, uint8 insideValue, uint8 outsideValue, const std::atomic_bool& shouldCancel,
                                                               const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::ThresholdMaxCCExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, imageGeom.getDimensions(), minimumObjectSizeInPixels, upperBoundary, insideValue,
                                            outsideValue,  shouldCancel,   messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

namespace detail
{
/**
 * @brief Functor that copies an integer marker array's values into a uint32 store (widening cast), dispatched on
 *        the source element type S via the IntegerOnly policy (which calls operator()<S>() through
 *        ExecuteDataFunctionIntType). The source is read through the SAFE typed accessor
 *        getDataRefAs<DataArray<S>> -- not a raw dynamic_cast -- so a type mismatch throws rather than being UB.
 *        Single-component is guaranteed by the caller's preflight, so getSize() (values) == the tuple count.
 */
struct WidenIntegerArrayToUInt32Fn
{
  const DataStructure& dataStructure;
  const DataPath& srcPath;
  AbstractDataStore<uint32>& dst;
  const std::atomic_bool& shouldCancel;

  template <class S>
  Result<> operator()() const
  {
    const AbstractDataStore<S>& s = dataStructure.getDataRefAs<DataArray<S>>(srcPath).getDataStoreRef();
    const usize total = s.getSize();
    constexpr usize k_ChunkValues = 65536;
    auto inBuf = std::make_unique<S[]>(std::min(k_ChunkValues, total));
    auto outBuf = std::make_unique<uint32[]>(std::min(k_ChunkValues, total));
    for(usize start = 0; start < total; start += k_ChunkValues)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_ChunkValues, total - start);
      if(Result<> r = s.copyIntoBuffer(start, nonstd::span<S>(inBuf.get(), count)); r.invalid())
      {
        return r;
      }
      for(usize i = 0; i < count; ++i)
      {
        outBuf[i] = static_cast<uint32>(inBuf[i]);
      }
      if(Result<> r = dst.copyFromBuffer(start, nonstd::span<const uint32>(outBuf.get(), count)); r.invalid())
      {
        return r;
      }
    }
    return {};
  }
};

/**
 * @brief Copies the integer marker array at @p srcPath (any of the 8 integer element types) into @p dst as uint32.
 *        The flood runs in uint32 (see @ref ApplyWatershedFromMarkers) but the marker array may be any integer
 *        type, so it is widened up front and the flood dispatches on the grayscale type only. Exact for all marker
 *        labels < 0xFFFFFFFF (i.e. <= 2^32 - 2; every realistic segmentation): 0xFFFFFFFF is reserved as the border
 *        sentinel, so a label of exactly 0xFFFFFFFF (or >= 2^32 on an int64/uint64 array, which would truncate) would
 *        alias it -- documented in the filter doc, tested by the direct-ITK grid at realistic labels.
 */
inline Result<> WidenIntegerArrayToUInt32(const DataStructure& dataStructure, const DataPath& srcPath, AbstractDataStore<uint32>& dst, const std::atomic_bool& shouldCancel)
{
  const auto& srcArray = dataStructure.getDataRefAs<IDataArray>(srcPath);
  return IntegerOnly::dispatch(srcArray.getDataType(), WidenIntegerArrayToUInt32Fn{dataStructure, srcPath, dst, shouldCancel});
}

/**
 * @brief Functor that copies a uint32 store's values into an integer output array (narrowing cast), dispatched on
 *        the destination element type D via the IntegerOnly policy. The destination is written through the SAFE
 *        typed accessor getDataRefAs<DataArray<D>> -- not a raw dynamic_cast. The output is created of the marker's
 *        type by the filter's preflight, so the flooded uint32 labels (all < 2^32 for realistic markers) round-trip.
 */
struct NarrowUInt32StoreToIntegerArrayFn
{
  const AbstractDataStore<uint32>& src;
  DataStructure& dataStructure;
  const DataPath& dstPath;
  const std::atomic_bool& shouldCancel;

  template <class D>
  Result<> operator()() const
  {
    AbstractDataStore<D>& d = dataStructure.getDataRefAs<DataArray<D>>(dstPath).getDataStoreRef();
    const usize total = src.getSize();
    constexpr usize k_ChunkValues = 65536;
    auto inBuf = std::make_unique<uint32[]>(std::min(k_ChunkValues, total));
    auto outBuf = std::make_unique<D[]>(std::min(k_ChunkValues, total));
    for(usize start = 0; start < total; start += k_ChunkValues)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_ChunkValues, total - start);
      if(Result<> r = src.copyIntoBuffer(start, nonstd::span<uint32>(inBuf.get(), count)); r.invalid())
      {
        return r;
      }
      for(usize i = 0; i < count; ++i)
      {
        outBuf[i] = static_cast<D>(inBuf[i]);
      }
      if(Result<> r = d.copyFromBuffer(start, nonstd::span<const D>(outBuf.get(), count)); r.invalid())
      {
        return r;
      }
    }
    return {};
  }
};

/**
 * @brief Narrows the uint32 flood output into the integer output array at @p dstPath (whose element type is the
 *        marker's type, set by the filter's preflight). Inverse of @ref WidenIntegerArrayToUInt32.
 */
inline Result<> NarrowUInt32StoreToIntegerArray(const AbstractDataStore<uint32>& src, DataStructure& dataStructure, const DataPath& dstPath, const std::atomic_bool& shouldCancel)
{
  const auto& dstArray = dataStructure.getDataRefAs<IDataArray>(dstPath);
  return IntegerOnly::dispatch(dstArray.getDataType(), NarrowUInt32StoreToIntegerArrayFn{src, dataStructure, dstPath, shouldCancel});
}

/**
 * @brief The border sentinel for the uint32 flood: static_cast<uint32>(NumericTraits<markerType>::max()). ITK's
 *        MorphologicalWatershedFromMarkers uses NumericTraits<TLabel>::max() as the out-of-bounds marker-image
 *        boundary constant; widening the flood to uint32 requires reproducing that per-type constant so the flood
 *        is bit-identical to ITK's native-TLabel flood (incl. the label==type-max collision). For uint32/int64/
 *        uint64 the max saturates to 0xFFFFFFFF on the cast, which is the intended sentinel for the uint32 flood.
 */
inline uint32 MarkerTypeMaxAsUInt32(DataType markerType)
{
  switch(markerType)
  {
  case DataType::int8:
    return static_cast<uint32>(std::numeric_limits<int8>::max());
  case DataType::uint8:
    return static_cast<uint32>(std::numeric_limits<uint8>::max());
  case DataType::int16:
    return static_cast<uint32>(std::numeric_limits<int16>::max());
  case DataType::uint16:
    return static_cast<uint32>(std::numeric_limits<uint16>::max());
  case DataType::int32:
    return static_cast<uint32>(std::numeric_limits<int32>::max());
  case DataType::uint32:
    return static_cast<uint32>(std::numeric_limits<uint32>::max());
  case DataType::int64:
    return static_cast<uint32>(std::numeric_limits<int64>::max());
  case DataType::uint64:
    return static_cast<uint32>(std::numeric_limits<uint64>::max());
  default:
    return std::numeric_limits<uint32>::max();
  }
}

/**
 * @brief Dispatched execute body for MorphologicalWatershedFromMarkers. Dispatches on the GRAYSCALE element type T
 *        (the FAH key). The all-OOC external endpoint streams native markers into its uint32 flood records and writes
 *        native output labels directly. Other endpoints retain the uint32 scratch-store path.
 */
struct MorphologicalWatershedFromMarkersExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;  // grayscale
  const DataPath& markerArrayPath; // integer labels
  const DataPath& outputArrayPath; // marker-type labels
  SizeVec3 dims;
  bool markWatershedLine;
  bool fullyConnected;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T> // T = grayscale type
  Result<> operator()() const
  {
    // CreateDataStoreWithFormat throws std::bad_alloc for an IN-CORE store on OOM (it does NOT return nullptr, so the
    // == nullptr checks below only catch an out-of-core failure). Resident flood buffers and the external flood's
    // bounded buffers/cache metadata can also throw. Convert either allocation failure into a filter error instead of
    // allowing an exception to escape this execute path.
    try
    {
      const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
      const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
      const auto& markerArray = dataStructure.getDataRefAs<IDataArray>(markerArrayPath);
      const std::string format = inArray.getDataFormat();
      const std::vector<usize> tupleShape = inArray.getTupleShape();
      auto markerU32 = DataStoreUtilities::CreateDataStoreWithFormat<uint32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
      auto outU32 = DataStoreUtilities::CreateDataStoreWithFormat<uint32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
      if(markerU32 == nullptr || outU32 == nullptr)
      {
        return MakeErrorResult(-79010, "MorphologicalWatershedFromMarkers: failed to allocate a uint32 scratch store.");
      }

      if(Result<> r = WidenIntegerArrayToUInt32(dataStructure, markerArrayPath, *markerU32, shouldCancel); r.invalid())
      {
        return r;
      }
      const uint32 borderSentinel = MarkerTypeMaxAsUInt32(markerArray.getDataType());
      if(Result<> r = ApplyWatershedFromMarkers<T>(inStore, *markerU32, *outU32, dims, markWatershedLine, fullyConnected, borderSentinel, shouldCancel, messageHandler); r.invalid())
      {
        return r;
      }
      return NarrowUInt32StoreToIntegerArray(*outU32, dataStructure, outputArrayPath, shouldCancel);
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult(-79045, "Morphological watershed could not allocate working storage for this volume. Resident execution requires its global working set in RAM; out-of-core execution "
                                     "requires bounded buffers and external temporary records.");
    }
  }
};

/**
 * @brief Dispatched execute body for MorphologicalWatershed -- the ITK composite (verified in
 *        itkMorphologicalWatershedImageFilter.hxx::GenerateData): (1) input2 = (Level != 0) ? HMinima(input, Level) :
 *        input; (2) rmin = RegionalMinima(input2) -> uint32 (foreground = uint32-max where a regional minimum,
 *        background = 0, flat-is-minima); (3) label = ConnectedComponent(rmin, predicate v != 0) -> uint32; (4) out =
 *        WatershedFromMarkers(input, label). Dispatches on the GRAYSCALE element type T only (the flood key); every
 *        intermediate is a scratch store that inherits the input's storage format, so an OOC input keeps the whole
 *        pipeline OOC-bounded. The output is a FIXED uint32 label image (matching the legacy ITK FilterOutputType), so
 *        the preflight must have created it via the AlwaysUInt32 map.
 */
struct MorphologicalWatershedExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  float64 level;
  bool markWatershedLine;
  bool fullyConnected;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T> // T = grayscale type
  Result<> operator()() const
  {
    // Every whole-volume scratch store below (HMinima, regional-minima, connected-component labels) is allocated by
    // CreateDataStoreWithFormat, which throws std::bad_alloc for an IN-CORE store on OOM rather than returning nullptr
    // (the == nullptr checks only catch an out-of-core failure). Resident flood buffers and the external flood's bounded
    // buffers/cache metadata can also throw. Convert either allocation failure into a filter error.
    try
    {
      const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
      const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
      auto& outArray = dataStructure.getDataRefAs<DataArray<uint32>>(outputArrayPath);
      AbstractDataStore<uint32>& outStore = outArray.getDataStoreRef();
      const std::string format = inArray.getDataFormat();
      const std::vector<usize> tupleShape = inArray.getTupleShape();

      // (1) input2 = (Level != 0) ? HMinima(input, Level) : input. HMinima = BuildReconstructionMarker(PlusHeight) +
      // ApplyMorphologicalReconstruction(Erosion) under the input as mask (the exact HMinimaImageFilter wiring). ITK
      // stores m_Level as InputImagePixelType, so the level is TRUNCATED to the input type T before BOTH the != gate
      // and the HMinima height (itkMorphologicalWatershedImageFilter.hxx). We must truncate to T here too: the marker
      // build computes trunc(v + level), which for integer T differs from ITK's v + (T)level when v + level < 0
      // (truncation rounds toward zero, not down) -- taking the height off levelT makes v + levelT integral so the
      // two agree exactly. The reconstruction @pre requires the output store distinct from BOTH the marker and the
      // mask, so hminStore is a separate scratch from markerScratch/inStore.
      const T levelT = static_cast<T>(level); // mirrors ITK's m_Level (InputImagePixelType) truncation
      const AbstractDataStore<T>* input2 = &inStore;
      std::shared_ptr<AbstractDataStore<T>> hminStore;
      if(levelT != T{})
      {
        auto markerScratch = DataStoreUtilities::CreateDataStoreWithFormat<T>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
        hminStore = DataStoreUtilities::CreateDataStoreWithFormat<T>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
        if(markerScratch == nullptr || hminStore == nullptr)
        {
          return MakeErrorResult(-79020, "MorphologicalWatershed: failed to allocate an HMinima scratch store.");
        }
        if(Result<> r = BuildReconstructionMarker<T>(inStore, *markerScratch, dims, ReconMarker::PlusHeight, static_cast<float64>(levelT), shouldCancel); r.invalid())
        {
          return r;
        }
        if(Result<> r = ApplyMorphologicalReconstruction<T>(*markerScratch, inStore, *hminStore, dims, ReconstructOp::Erosion, fullyConnected, inArray, outArray, shouldCancel, messageHandler);
           r.invalid())
        {
          return r;
        }
        input2 = hminStore.get();
      }

      // (2) rmin = RegionalMinima(input2) -> uint32 (foreground = uint32-max where a regional minimum, background = 0).
      // Replicates RegionalExtremaExecuteFn (Minima, flatIsExtremum=true): a completely flat image is all-minima (ITK's
      // GetFlat() branch with SetFlatIsMinima(true)); otherwise valued-extrema into a T scratch, then threshold (the
      // non-extrema were set to the type-max marker -> background 0, the extrema kept their value -> foreground).
      auto rminBinary = DataStoreUtilities::CreateDataStoreWithFormat<uint32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
      if(rminBinary == nullptr)
      {
        return MakeErrorResult(-79021, "MorphologicalWatershed: failed to allocate a regional-minima scratch store.");
      }
      {
        // Only min/max feed the flat-image check below, so the parallel min/max reduction replaces the full
        // statistics pass.
        Result<ArrayMinMax<T>> stats = ComputeArrayMinMax<T>(*input2, shouldCancel);
        if(stats.invalid())
        {
          return {nonstd::make_unexpected(std::move(stats.errors()))};
        }
        if(shouldCancel)
        {
          return {};
        }
        if(stats.value().min == stats.value().max)
        {
          if(Result<> r = FillStore<uint32>(*rminBinary, std::numeric_limits<uint32>::max(), shouldCancel); r.invalid()) // flat image -> all minima
          {
            return r;
          }
        }
        else
        {
          auto rminValued = DataStoreUtilities::CreateDataStoreWithFormat<T>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
          if(rminValued == nullptr)
          {
            return MakeErrorResult(-79022, "MorphologicalWatershed: failed to allocate a valued regional-minima scratch store.");
          }
          if(Result<> r = ApplyValuedRegionalExtrema<T>(*input2, *rminValued, dims, RegionalExtremaOp::Minima, fullyConnected, inArray, outArray, shouldCancel, messageHandler); r.invalid())
          {
            return r;
          }
          const T markerValue = std::numeric_limits<T>::max(); // the Minima marker (non-extrema pixels were set to type max)
          const usize total = inStore.getSize();
          constexpr usize k_ChunkValues = 65536;
          auto valuedBuf = std::make_unique<T[]>(std::min(k_ChunkValues, total));
          auto binaryBuf = std::make_unique<uint32[]>(std::min(k_ChunkValues, total));
          for(usize start = 0; start < total; start += k_ChunkValues)
          {
            if(shouldCancel)
            {
              return {};
            }
            const usize count = std::min(k_ChunkValues, total - start);
            if(Result<> r = rminValued->copyIntoBuffer(start, nonstd::span<T>(valuedBuf.get(), count)); r.invalid())
            {
              return r;
            }
            for(usize i = 0; i < count; ++i)
            {
              binaryBuf[i] = (valuedBuf[i] == markerValue) ? 0u : std::numeric_limits<uint32>::max();
            }
            if(Result<> r = rminBinary->copyFromBuffer(start, nonstd::span<const uint32>(binaryBuf.get(), count)); r.invalid())
            {
              return r;
            }
          }
        }
      }
      hminStore.reset(); // input2 (which may alias *hminStore) is not read past step 2 -- release the HMinima T-image now (no-op when Level==0)

      // (3) label = ConnectedComponent(rmin, predicate v != 0) -> uint32 markers (one consecutive label per
      // regional-minima component, background 0), the seeds the flood floods from.
      auto labelStore = DataStoreUtilities::CreateDataStoreWithFormat<uint32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
      if(labelStore == nullptr)
      {
        return MakeErrorResult(-79023, "MorphologicalWatershed: failed to allocate a connected-component label scratch store.");
      }
      if(Result<> r = LabelConnectedComponents<uint32>(*rminBinary, *labelStore, dims, [](uint32 v) { return v != 0u; }, fullyConnected, shouldCancel, messageHandler); r.invalid())
      {
        return r;
      }

      // (4) out = WatershedFromMarkers(input, label). The markers are uint32 connected-component labels (all < 2^32),
      // so the border sentinel is uint32-max -- no real label can collide with it.
      rminBinary.reset(); // consumed by LabelConnectedComponents (step 3); release it before the flood allocates resident state or external records
      return ApplyWatershedFromMarkers<T>(inStore, *labelStore, outStore, dims, markWatershedLine, fullyConnected, std::numeric_limits<uint32>::max(), shouldCancel, messageHandler);
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult(-79046, "Morphological watershed could not allocate working storage for this volume. Resident execution requires its global working set in RAM; out-of-core execution "
                                     "requires bounded buffers and external temporary records.");
    }
  }
};
} // namespace detail

/**
 * @brief Execute the (ITK-free, out-of-core) MorphologicalWatershedFromMarkers filter: a marker-controlled
 *        hierarchical-queue (FAH) watershed flood (see @ref ApplyWatershedFromMarkers). Dispatches on the grayscale
 *        input element type via @p TypeSetPolicy (AllNumeric by default -- all 10 scalar types); the marker array is
 *        any integer type and the output is created of the marker's type by the filter's preflight. Reproduces legacy
 *        ITK's MorphologicalWatershedFromMarkersImageFilter output EXACTLY, including label values (see the Parity
 *        model and the direct-ITK oracle test). The caller's preflight must have validated the grayscale (scalar,
 *        allowed type, tuple==cells) and the marker (integer scalar, tuple==grayscale).
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteMorphologicalWatershedFromMarkersImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& markerArrayPath,
                                                             const DataPath& outputArrayPath, bool markWatershedLine, bool fullyConnected, const std::atomic_bool& shouldCancel,
                                                             const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::MorphologicalWatershedFromMarkersExecuteFn executeFn{dataStructure,     inputArrayPath, markerArrayPath, outputArrayPath, imageGeom.getDimensions(),
                                                               markWatershedLine, fullyConnected, shouldCancel,    messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

/**
 * @brief Execute the (ITK-free, out-of-core) MorphologicalWatershed filter: the ITK composite (HMinima ->
 *        RegionalMinima -> ConnectedComponent -> marker-controlled FAH watershed flood; see @ref
 *        MorphologicalWatershedExecuteFn), reproducing legacy ITK's MorphologicalWatershedImageFilter output EXACTLY
 *        (label values included; see the Parity model and the live-ITK test). Dispatches on the grayscale input
 *        element type via @p TypeSetPolicy (AllNumeric by default -- all 10 scalar types); the output is a FIXED
 *        uint32 label image created by the filter's preflight (AlwaysUInt32). The caller's preflight must have
 *        validated the input (scalar, allowed type, tuple==cells).
 */
template <class TypeSetPolicy = AllNumeric>
Result<> ExecuteMorphologicalWatershedImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, float64 level,
                                                  bool markWatershedLine, bool fullyConnected, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::MorphologicalWatershedExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, imageGeom.getDimensions(), level, markWatershedLine, fullyConnected, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}
} // namespace nx::core::ImageProcessing
