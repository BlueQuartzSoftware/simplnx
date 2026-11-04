#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/MoveDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fmt/format.h>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
/**
 * @brief Adds the selected geometry path to layout errors at a filter boundary.
 * @param result Layout validation result. Error codes and warnings remain unchanged.
 * @param imageGeomPath Selected Image Geometry path.
 * @return Result with quoted geometry context on each error, or the unchanged valid result.
 */
inline Result<> AddMorphologyLayoutGeometryContext(Result<> result, const DataPath& imageGeomPath)
{
  if(result.invalid())
  {
    const auto context = fmt::format(" Image Geometry: '{}'.", imageGeomPath.toString());
    for(auto& error : result.errors())
    {
      error.message += context;
    }
  }
  return result;
}

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
 *            original geometry, any OTHER arrays in its cell data and unpreserved geometry children are
 *            discarded; a warning enumerates them.
 *          - copyNonCellAttributeMatrices == true: non-cell Attribute Matrices are moved before the deferred
 *            in-place swap (no array copies), or deep-copied into the new geometry in non-in-place mode.
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
                                              const std::string& outputGeomName, const std::string& outputArrayName, bool copyNonCellAttributeMatrices = false)
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

  if(copyNonCellAttributeMatrices)
  {
    for(const auto& [childId, childObject] : imageGeom)
    {
      if(childId == cellData->getId() || dynamic_cast<const AttributeMatrix*>(childObject.get()) == nullptr)
      {
        continue;
      }
      const DataPath sourcePath = imageGeomPath.createChildPath(childObject->getName());
      if(performInPlace)
      {
        // Preserve the original objects and stores after projection, before deleting their old geometry.
        result.value().appendDeferredAction(std::make_unique<MoveDataAction>(sourcePath, targetGeomPath));
      }
      else
      {
        const DataPath destinationPath = targetGeomPath.createChildPath(childObject->getName());
        std::vector<DataPath> allCreatedPaths = {destinationPath};
        const auto childPaths = GetAllChildDataPathsRecursive(dataStructure, sourcePath);
        if(childPaths.has_value())
        {
          for(const auto& childPath : childPaths.value())
          {
            allCreatedPaths.push_back(childPath.rebase(sourcePath, destinationPath).value());
          }
        }
        // Data-store deep copies use bounded transfers and respect the destination storage policy.
        result.value().appendAction(std::make_unique<CopyDataObjectAction>(sourcePath, destinationPath, allCreatedPaths));
      }
    }
  }

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

    // In-place discards the original geometry. List unpreserved children as well as other cell-data
    // arrays. Non-cell Attribute Matrices can be moved above. The selected input is consumed by the
    // projection and the output name is replaced, so neither is listed.
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
      discarded += imageGeom.getCellDataPath().createChildPath(childName).toString();
    }
    for(const auto& [childId, childObject] : imageGeom)
    {
      if(childId == cellData->getId() || (copyNonCellAttributeMatrices && dynamic_cast<const AttributeMatrix*>(childObject.get()) != nullptr))
      {
        continue;
      }
      if(!discarded.empty())
      {
        discarded += ", ";
      }
      discarded += imageGeomPath.createChildPath(childObject->getName()).toString();
    }
    if(!discarded.empty())
    {
      result.warnings().push_back(Warning{-8343, fmt::format("Perform In-Place replaces Image Geometry '{}' with the collapsed projection. The following other data object(s), including their "
                                                             "contents, will be discarded: {}. Disable 'Perform In-Place' to keep the original geometry.",
                                                             imageGeomPath.toString(), discarded)});
    }
  }

  return result;
}

} // namespace nx::core::ImageProcessing
