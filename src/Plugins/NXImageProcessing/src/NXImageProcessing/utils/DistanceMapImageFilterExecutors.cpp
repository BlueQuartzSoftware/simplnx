#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "NXImageProcessing/utils/ImageFilterExecutorDetail.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/DanielssonDistanceMapEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/FastChamferDistanceEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/IsoContourDistanceEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/MaurerDistanceMapEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologyEngine.hpp"

#include <atomic>
#include <cmath>
#include <string>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
namespace
{
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

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteSignedMaurerDistanceMapImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, bool insideIsPositive,
                                                   bool squaredDistance, bool useSpacing, float64 backgroundValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = IntegerOnly;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::SignedMaurerDistanceMapExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims,         insideIsPositive, squaredDistance,
                                                     useSpacing,    spacing,        backgroundValue, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteDanielssonDistanceMapImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, bool inputIsBinary,
                                                 bool squaredDistance, bool useSpacing, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = IntegerOnly;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::DanielssonDistanceMapExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, inputIsBinary, squaredDistance, useSpacing, spacing, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteSignedDanielssonDistanceMapImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                       bool insideIsPositive, bool squaredDistance, bool useSpacing, const std::atomic_bool& shouldCancel,
                                                       const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = IntegerOnly;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::SignedDanielssonDistanceMapExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, insideIsPositive, squaredDistance, useSpacing, spacing, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteIsoContourDistanceImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, float64 levelSetValue,
                                              float64 farValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::IsoContourDistanceExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, levelSetValue, farValue, spacing, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteApproximateSignedDistanceMapImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                        float64 insideValue, float64 outsideValue, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = IntegerOnly;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::ApproximateSignedDistanceMapExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, insideValue, outsideValue, spacing, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
