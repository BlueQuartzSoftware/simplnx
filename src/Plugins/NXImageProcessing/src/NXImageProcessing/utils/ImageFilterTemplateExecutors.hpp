#pragma once

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageProcessing/AxisProjectionEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/BoxNeighborhoodEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ContourEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/FiniteDifferenceEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/HistogramEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/PointwiseEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StreamingStatistics.hpp"

#include <array>
#include <atomic>
#include <type_traits>
#include <vector>

namespace nx::core::ImageProcessing
{
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

} // namespace nx::core::ImageProcessing
