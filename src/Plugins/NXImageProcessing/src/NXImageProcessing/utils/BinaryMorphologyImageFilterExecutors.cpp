#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "NXImageProcessing/utils/ImageFilterExecutorDetail.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologyEngine.hpp"

#include <array>
#include <atomic>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
namespace
{
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

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteBinaryMorphologyImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const StructuringElement& se,
                                            MorphOp op, float64 foreground, float64 background, bool boundaryToForeground, const std::atomic_bool& shouldCancel,
                                            const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = IntegerOnly;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  if(auto layout = ValidateMorphologyLayout(dims, se, false, GetDataTypeSize(inputType)); layout.invalid())
  {
    return AddMorphologyLayoutGeometryContext(std::move(layout), imageGeomPath);
  }

  detail::BinaryMorphologyExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, se, op, foreground, background, boundaryToForeground, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteBinaryMorphologyCompositeImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                     const StructuringElement& se, BinaryCompositeOp op, float64 foreground, float64 background, bool safeBorder, const std::atomic_bool& shouldCancel,
                                                     const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = IntegerOnly;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  if(auto layout = ValidateMorphologyLayout(dims, se, safeBorder && op == BinaryCompositeOp::Closing, GetDataTypeSize(inputType)); layout.invalid())
  {
    return AddMorphologyLayoutGeometryContext(std::move(layout), imageGeomPath);
  }

  detail::BinaryMorphologyCompositeExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, se, op, foreground, background, safeBorder, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
