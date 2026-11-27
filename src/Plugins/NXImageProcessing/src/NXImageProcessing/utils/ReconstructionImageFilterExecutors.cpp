#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "NXImageProcessing/utils/HConvexUtilities.hpp"
#include "NXImageProcessing/utils/ImageFilterExecutorDetail.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologyEngine.hpp"

#include <algorithm>
#include <atomic>
#include <limits>
#include <memory>
#include <nonstd/span.hpp>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
namespace
{
/**
 * @brief Dispatched execute body for the grayscale reconstruction filters. Derives the marker (@ref
 *        BuildReconstructionMarker) into an out-of-core-inheriting scratch store, then reconstructs it under the
 *        original input (the mask) into the output. The engine's DispatchAlgorithm keys the in-core/out-of-core
 *        path on the real input + output arrays (the scratch marker inherits the input's storage mode), so an
 *        out-of-core input drives the streamed sweep end-to-end. Output type == input type (SameAsInput).
 */
struct MorphologicalReconstructionExecuteFn
{
  DataStructure& data_structure;
  const DataPath& input_array_path;
  const DataPath& output_array_path;
  SizeVec3Type dims;
  ReconstructOp op;
  bool fully_connected;
  ReconMarker marker_kind;
  ReconstructionHeightType height;
  const std::atomic_bool& should_cancel;
  const IFilter::MessageHandler& message_handler;
  bool difference_from_input; // HConvex: after reconstruction, output = input - reconstruction (the h-dome)

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = data_structure.getDataRefAs<DataArray<T>>(input_array_path);
    auto& outArray = data_structure.getDataRefAs<DataArray<T>>(output_array_path);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<T>& outStore = outArray.getDataStoreRef();

    std::shared_ptr<AbstractDataStore<T>> markerStore;
    const AbstractDataStore<T>* markerPtr = &inStore;
    ReconstructionMarkerOptions markerOptions;
    const bool isHeightMarker = marker_kind == ReconMarker::MinusHeight || marker_kind == ReconMarker::PlusHeight;
    const bool isBorderMarker = marker_kind == ReconMarker::FillholeMaxBorder || marker_kind == ReconMarker::GrindPeakMinBorder;
    const bool useDerivedMarker = isBorderMarker || (inStore.getStoreType() == IDataStore::StoreType::OutOfCore && isHeightMarker);
    if(useDerivedMarker)
    {
      if(marker_kind == ReconMarker::MinusHeight || marker_kind == ReconMarker::PlusHeight)
      {
        markerOptions.source = marker_kind == ReconMarker::MinusHeight ? ReconstructionMarkerSource::MaskMinusHeight : ReconstructionMarkerSource::MaskPlusHeight;
      }
      else
      {
        markerOptions.source = marker_kind == ReconMarker::FillholeMaxBorder ? ReconstructionMarkerSource::MaskWithMaximumInterior : ReconstructionMarkerSource::MaskWithMinimumInterior;
      }
      markerOptions.height = height;
    }
    else
    {
      // Marker scratch inherits the input's storage format (out-of-core input -> out-of-core scratch), keeping the
      // whole filter memory-bounded through the streamed sweep path.
      markerStore = data_store_utilities::CreateDataStoreWithFormat<T>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
      if(auto markerResult = BuildReconstructionMarker<T>(inStore, *markerStore, dims, marker_kind, height, should_cancel); markerResult.invalid())
      {
        return markerResult;
      }
      markerPtr = markerStore.get();
    }
    // Mask == the ORIGINAL input; the engine reconstructs the marker under it into outStore. OOC detection uses
    // the real input (mask) + output arrays (the scratch marker has no IDataArray).
    const bool useTemporaryWork = marker_kind == ReconMarker::FillholeMaxBorder || marker_kind == ReconMarker::GrindPeakMinBorder;
    auto reconstructionResult =
        ApplyMorphologicalReconstruction<T>(*markerPtr, inStore, outStore, dims, op, fully_connected, inArray, outArray, should_cancel, message_handler, useTemporaryWork, markerOptions);
    if(reconstructionResult.invalid())
    {
      return reconstructionResult;
    }
    if(difference_from_input)
    {
      // HConvex owns exact integer markers and checked signed contrast. Other callers retain their existing defaults.
      return MergeResults(std::move(reconstructionResult), SubtractHConvexContrast<T>(inStore, outStore, outStore, input_array_path, should_cancel));
    }
    return reconstructionResult;
  }
};

Result<> ExecutePreparedMorphologicalReconstruction(DataStructure& dataStructure, const DataPath& imageGeomPath, DataType inputType, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                    ReconstructOp operation, bool fullyConnected, ReconMarker markerKind, ReconstructionHeightType height, const std::atomic_bool& shouldCancel,
                                                    const IFilter::MessageHandler& messageHandler, bool differenceFromInput)
{
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const MorphologicalReconstructionExecuteFn executeFn{
      .data_structure = dataStructure,
      .input_array_path = inputArrayPath,
      .output_array_path = outputArrayPath,
      .dims = imageGeom.getDimensions(),
      .op = operation,
      .fully_connected = fullyConnected,
      .marker_kind = markerKind,
      .height = height,
      .should_cancel = shouldCancel,
      .message_handler = messageHandler,
      .difference_from_input = differenceFromInput,
  };
  return AllNumeric::Dispatch(inputType, executeFn);
}

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
  DataStructure& data_structure;
  const DataPath& input_array_path;
  const DataPath& output_array_path;
  SizeVec3Type dims;
  const StructuringElement& se;
  MorphOp first_op;
  bool fully_connected;
  bool preserve_intensities;
  const std::atomic_bool& should_cancel;
  const IFilter::MessageHandler& message_handler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = data_structure.getDataRefAs<DataArray<T>>(input_array_path);
    auto& outArray = data_structure.getDataRefAs<DataArray<T>>(output_array_path);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<T>& outStore = outArray.getDataStoreRef();

    const ReconstructOp reconOp = (first_op == MorphOp::Erode) ? ReconstructOp::Dilation : ReconstructOp::Erosion;

    // Step 1: grayscale erode (Opening) / dilate (Closing) into an OOC-inheriting scratch.
    auto morph = data_store_utilities::CreateDataStoreWithFormat<T>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    if(Result<> r = ApplyMorphology<T>(inStore, *morph, dims, se, first_op, inArray, outArray, should_cancel, message_handler); r.invalid())
    {
      return r;
    }

    if(!preserve_intensities)
    {
      // Reconstruct the morphology result under the input directly into the output.
      return ApplyMorphologicalReconstruction<T>(*morph, inStore, outStore, dims, reconOp, fully_connected, inArray, outArray, should_cancel, message_handler);
    }

    // PreserveIntensities: reconstruct into a scratch, re-seed the original input where the first reconstruction was
    // a fixpoint of the morphology result, then reconstruct that marker into the output.
    auto recon = data_store_utilities::CreateDataStoreWithFormat<T>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    if(Result<> r = ApplyMorphologicalReconstruction<T>(*morph, inStore, *recon, dims, reconOp, fully_connected, inArray, outArray, should_cancel, message_handler); r.invalid())
    {
      return r;
    }
    // Build the compare-select marker in place into *recon, aliased as both the read-only reconstruction operand and
    // the output. BuildPreserveIntensitiesMarker copies each chunk into local buffers before writing that chunk back,
    // so the aliasing is safe (reads precede the write per chunk) and it avoids a third full-volume scratch. Neither
    // *morph nor the pre-marker contents of *recon are needed after this pass.
    const T unmarkedFill = (first_op == MorphOp::Erode) ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
    if(Result<> r = BuildPreserveIntensitiesMarker<T>(*morph, *recon, inStore, *recon, unmarkedFill, should_cancel); r.invalid())
    {
      return r;
    }
    return ApplyMorphologicalReconstruction<T>(*recon, inStore, outStore, dims, reconOp, fully_connected, inArray, outArray, should_cancel, message_handler);
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
  DataStructure& data_structure;
  const DataPath& input_array_path;
  const DataPath& output_array_path;
  SizeVec3Type dims;
  const StructuringElement& se;
  float64 foreground;
  float64 background;
  bool fully_connected;
  const std::atomic_bool& should_cancel;
  const IFilter::MessageHandler& message_handler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = data_structure.getDataRefAs<DataArray<T>>(input_array_path);
    auto& outArray = data_structure.getDataRefAs<DataArray<T>>(output_array_path);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<T>& outStore = outArray.getDataStoreRef();

    const T foregroundT = static_cast<T>(foreground);
    const T backgroundT = static_cast<T>(background);

    // Enforce the strictly-{fg, bg} binary-input contract (as the standalone binary morphology filters do).
    if(Result<> r = ScanBinaryInput<T>(inStore, foregroundT, backgroundT, input_array_path, should_cancel); r.invalid())
    {
      return r;
    }

    // Binary erosion into an OOC-inheriting scratch. boundaryToForeground=true matches ITK's internal BinaryErode.
    auto erode = data_store_utilities::CreateDataStoreWithFormat<T>(inArray.getDataFormat(), inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    if(Result<> r = ApplyBinaryMorphology<T>(inStore, *erode, dims, se, MorphOp::Erode, foregroundT, backgroundT, /*boundaryToForeground=*/true, inArray, outArray, should_cancel, message_handler);
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
    return ApplyMorphologicalReconstruction<T>(*erode, inStore, outStore, dims, op, fully_connected, inArray, outArray, should_cancel, message_handler);
  }
};

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteMorphologicalReconstructionImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, ReconstructOp op,
                                                       bool fullyConnected, detail::ReconMarker markerKind, float64 height, const std::atomic_bool& shouldCancel,
                                                       const IFilter::MessageHandler& messageHandler, bool differenceFromInput)
{
  const auto inputType = dataStructure.getDataRefAs<IDataArray>(inputArrayPath).getDataType();
  return detail::ExecutePreparedMorphologicalReconstruction(dataStructure, imageGeomPath, inputType, inputArrayPath, outputArrayPath, op, fullyConnected, markerKind, height, shouldCancel,
                                                            messageHandler, differenceFromInput);
}

// -----------------------------------------------------------------------------
Result<> detail::ExecuteMorphologicalReconstructionWithValidatedHeight(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                       ReconstructOp operation, bool fullyConnected, ReconMarker markerKind, float64 height, const std::atomic_bool& shouldCancel,
                                                                       const IFilter::MessageHandler& messageHandler, bool differenceFromInput)
{
  const auto inputType = dataStructure.getDataRefAs<IDataArray>(inputArrayPath).getDataType();
  const ReconstructionHeightType offset =
      GetIntegerScalarTypes().contains(inputType) ? ReconstructionHeightType{ExactReconstructionHeight{SaturateCastFromDouble<uint64>(height)}} : ReconstructionHeightType{height};
  return ExecutePreparedMorphologicalReconstruction(dataStructure, imageGeomPath, inputType, inputArrayPath, outputArrayPath, operation, fullyConnected, markerKind, offset, shouldCancel,
                                                    messageHandler, differenceFromInput);
}

// -----------------------------------------------------------------------------
Result<> ExecuteReconstructionOpenCloseImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                   const StructuringElement& se, MorphOp firstOp, bool fullyConnected, bool preserveIntensities, const std::atomic_bool& shouldCancel,
                                                   const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicyType = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3Type dims = imageGeom.getDimensions();
  if(auto layout = ValidateMorphologyLayout(dims, se, false, GetDataTypeSize(inputType)); layout.invalid())
  {
    return AddMorphologyLayoutGeometryContext(std::move(layout), imageGeomPath);
  }

  detail::ReconstructionOpenCloseExecuteFn executeFn{.data_structure = dataStructure,
                                                     .input_array_path = inputArrayPath,
                                                     .output_array_path = outputArrayPath,
                                                     .dims = dims,
                                                     .se = se,
                                                     .first_op = firstOp,
                                                     .fully_connected = fullyConnected,
                                                     .preserve_intensities = preserveIntensities,
                                                     .should_cancel = shouldCancel,
                                                     .message_handler = messageHandler};
  return TypeSetPolicyType::Dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteBinaryOpeningByReconstructionImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                         const StructuringElement& se, float64 foreground, float64 background, bool fullyConnected, const std::atomic_bool& shouldCancel,
                                                         const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicyType = IntegerOnly;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3Type dims = imageGeom.getDimensions();
  if(auto layout = ValidateMorphologyLayout(dims, se, false, GetDataTypeSize(inputType)); layout.invalid())
  {
    return AddMorphologyLayoutGeometryContext(std::move(layout), imageGeomPath);
  }

  detail::BinaryReconstructionOpeningExecuteFn executeFn{.data_structure = dataStructure,
                                                         .input_array_path = inputArrayPath,
                                                         .output_array_path = outputArrayPath,
                                                         .dims = dims,
                                                         .se = se,
                                                         .foreground = foreground,
                                                         .background = background,
                                                         .fully_connected = fullyConnected,
                                                         .should_cancel = shouldCancel,
                                                         .message_handler = messageHandler};
  return TypeSetPolicyType::Dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
