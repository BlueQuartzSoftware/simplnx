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

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteMorphologyImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const StructuringElement& se,
                                      MorphOp op, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  if(auto layout = ValidateMorphologyLayout(dims, se, false, GetDataTypeSize(inputType)); layout.invalid())
  {
    return AddMorphologyLayoutGeometryContext(std::move(layout), imageGeomPath);
  }

  detail::MorphologyExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, se, op, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteMorphologyCompositeImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                               const StructuringElement& se, MorphCompositeOp op, bool safeBorder, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  if(auto layout = ValidateMorphologyLayout(dims, se, safeBorder && op != MorphCompositeOp::Gradient, GetDataTypeSize(inputType)); layout.invalid())
  {
    return AddMorphologyLayoutGeometryContext(std::move(layout), imageGeomPath);
  }

  detail::MorphologyCompositeExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, se, op, safeBorder, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
