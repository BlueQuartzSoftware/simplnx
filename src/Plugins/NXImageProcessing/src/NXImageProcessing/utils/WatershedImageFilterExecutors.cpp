#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "NXImageProcessing/utils/ImageFilterExecutorDetail.hpp"
#include "NXImageProcessing/utils/WatershedMarkerConversion.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ConnectedComponentEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/RegionalExtremaEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/WatershedFromMarkersEngine.hpp"

#include <algorithm>
#include <atomic>
#include <limits>
#include <memory>
#include <new>
#include <nonstd/expected.hpp>
#include <nonstd/span.hpp>
#include <string>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
namespace
{
/**
 * @struct MorphologicalWatershedFromMarkersExecuteFn
 * @brief Dispatches the flood on the grayscale type after marker conversion and retains the diagnostics from each phase.
 */
struct MorphologicalWatershedFromMarkersExecuteFn
{
  DataStructure& data_structure;
  const DataPath& input_array_path;  // grayscale
  const DataPath& marker_array_path; // integer labels
  const DataPath& output_array_path; // marker-type labels
  SizeVec3Type dims;
  bool mark_watershed_line;
  bool fully_connected;
  const std::atomic_bool& should_cancel;
  const IFilter::MessageHandler& message_handler;
  MarkerIdConversionHandling conversionHandling = MarkerIdConversionHandling::Error;

  template <class T> // T = grayscale type
  Result<> operator()() const
  {
    // CreateDataStoreWithFormat throws std::bad_alloc for an IN-CORE store on OOM (it does NOT return nullptr, so the
    // == nullptr checks below only catch an out-of-core failure). Resident flood buffers and the external flood's
    // bounded buffers/cache metadata can also throw. Convert either allocation failure into a filter error instead of
    // allowing an exception to escape this execute path.
    Result<> accumulated;
    try
    {
      const auto& inArray = data_structure.getDataRefAs<DataArray<T>>(input_array_path);
      const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
      const auto& markerArray = data_structure.getDataRefAs<IDataArray>(marker_array_path);
      const std::string format = inArray.getDataFormat();
      const std::vector<usize> tupleShape = inArray.getTupleShape();
      auto markerU32 = data_store_utilities::CreateDataStoreWithFormat<uint32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
      auto outU32 = data_store_utilities::CreateDataStoreWithFormat<uint32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
      if(markerU32 == nullptr || outU32 == nullptr)
      {
        return MakeErrorResult(-79010, "MorphologicalWatershedFromMarkers: failed to allocate a uint32 scratch store.");
      }

      accumulated = WidenIntegerArrayToUInt32(data_structure, marker_array_path, *markerU32, should_cancel, conversionHandling);
      if(accumulated.invalid() || should_cancel)
      {
        return accumulated;
      }
      const uint32 borderSentinel = MarkerTypeMaxAsUInt32(markerArray.getDataType());
      auto floodResult = ApplyWatershedFromMarkers<T>(inStore, *markerU32, *outU32, dims, mark_watershed_line, fully_connected, borderSentinel, should_cancel, message_handler);
      accumulated = MergeResults(std::move(accumulated), std::move(floodResult));
      if(accumulated.invalid() || should_cancel)
      {
        return accumulated;
      }
      auto restoreResult = NarrowUInt32StoreToIntegerArray(*outU32, data_structure, output_array_path, should_cancel);
      return MergeResults(std::move(accumulated), std::move(restoreResult));
    } catch(const std::bad_alloc&)
    {
      return MergeResults(
          std::move(accumulated),
          MakeErrorResult(-79045, "Morphological watershed could not allocate working storage for this volume. Resident execution requires its global working set in RAM; out-of-core execution "
                                  "requires bounded buffers and external temporary records."));
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
 *        the preflight must have created it via the AlwaysUInt32Type map.
 */
struct MorphologicalWatershedExecuteFn
{
  DataStructure& data_structure;
  const DataPath& input_array_path;
  const DataPath& output_array_path;
  SizeVec3Type dims;
  float64 level;
  bool mark_watershed_line;
  bool fully_connected;
  const std::atomic_bool& should_cancel;
  const IFilter::MessageHandler& message_handler;

  template <class T> // T = grayscale type
  Result<> operator()() const
  {
    // Every whole-volume scratch store below (HMinima, regional-minima, connected-component labels) is allocated by
    // CreateDataStoreWithFormat, which throws std::bad_alloc for an IN-CORE store on OOM rather than returning nullptr
    // (the == nullptr checks only catch an out-of-core failure). Resident flood buffers and the external flood's bounded
    // buffers/cache metadata can also throw. Convert either allocation failure into a filter error.
    try
    {
      const auto& inArray = data_structure.getDataRefAs<DataArray<T>>(input_array_path);
      const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
      auto& outArray = data_structure.getDataRefAs<DataArray<uint32>>(output_array_path);
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
      const AbstractDataStore<T>* input2Ptr = &inStore;
      std::shared_ptr<AbstractDataStore<T>> hminStore;
      if(levelT != T{})
      {
        auto markerScratch = data_store_utilities::CreateDataStoreWithFormat<T>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
        hminStore = data_store_utilities::CreateDataStoreWithFormat<T>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
        if(markerScratch == nullptr || hminStore == nullptr)
        {
          return MakeErrorResult(-79020, "MorphologicalWatershed: failed to allocate an HMinima scratch store.");
        }
        if(Result<> r = BuildReconstructionMarker<T>(inStore, *markerScratch, dims, ReconMarker::PlusHeight, static_cast<float64>(levelT), should_cancel); r.invalid())
        {
          return r;
        }
        if(Result<> r = ApplyMorphologicalReconstruction<T>(*markerScratch, inStore, *hminStore, dims, ReconstructOp::Erosion, fully_connected, inArray, outArray, should_cancel, message_handler);
           r.invalid())
        {
          return r;
        }
        input2Ptr = hminStore.get();
      }

      // (2) rmin = RegionalMinima(input2) -> uint32 (foreground = uint32-max where a regional minimum, background = 0).
      // Replicates RegionalExtremaExecuteFn (Minima, flatIsExtremum=true): a completely flat image is all-minima (ITK's
      // GetFlat() branch with SetFlatIsMinima(true)); otherwise valued-extrema into a T scratch, then threshold (the
      // non-extrema were set to the type-max marker -> background 0, the extrema kept their value -> foreground).
      auto rminBinary = data_store_utilities::CreateDataStoreWithFormat<uint32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
      if(rminBinary == nullptr)
      {
        return MakeErrorResult(-79021, "MorphologicalWatershed: failed to allocate a regional-minima scratch store.");
      }
      {
        // Only min/max feed the flat-image check below, so the parallel min/max reduction replaces the full
        // statistics pass.
        Result<ArrayMinMax<T>> stats = ComputeArrayMinMax<T>(*input2Ptr, should_cancel);
        if(stats.invalid())
        {
          return {nonstd::make_unexpected(std::move(stats.errors()))};
        }
        if(should_cancel)
        {
          return {};
        }
        if(stats.value().min == stats.value().max)
        {
          if(Result<> r = FillStore<uint32>(*rminBinary, std::numeric_limits<uint32>::max(), should_cancel); r.invalid()) // flat image -> all minima
          {
            return r;
          }
        }
        else
        {
          auto rminValued = data_store_utilities::CreateDataStoreWithFormat<T>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
          if(rminValued == nullptr)
          {
            return MakeErrorResult(-79022, "MorphologicalWatershed: failed to allocate a valued regional-minima scratch store.");
          }
          if(Result<> r = ApplyValuedRegionalExtrema<T>(*input2Ptr, *rminValued, dims, RegionalExtremaOp::Minima, fully_connected, inArray, outArray, should_cancel, message_handler); r.invalid())
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
            if(should_cancel)
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
      auto labelStore = data_store_utilities::CreateDataStoreWithFormat<uint32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
      if(labelStore == nullptr)
      {
        return MakeErrorResult(-79023, "MorphologicalWatershed: failed to allocate a connected-component label scratch store.");
      }
      if(Result<> r = LabelConnectedComponents<uint32>(*rminBinary, *labelStore, dims, [](uint32 v) { return v != 0u; }, fully_connected, should_cancel, message_handler); r.invalid())
      {
        return r;
      }

      // (4) Flood from the uint32 connected-component labels. The border value applies only to out-of-bounds neighbors.
      rminBinary.reset(); // consumed by LabelConnectedComponents (step 3); release it before the flood allocates resident state or external records
      return ApplyWatershedFromMarkers<T>(inStore, *labelStore, outStore, dims, mark_watershed_line, fully_connected, std::numeric_limits<uint32>::max(), should_cancel, message_handler);
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult(-79046, "Morphological watershed could not allocate working storage for this volume. Resident execution requires its global working set in RAM; out-of-core execution "
                                     "requires bounded buffers and external temporary records.");
    }
  }
};

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteMorphologicalWatershedFromMarkersImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& markerArrayPath,
                                                             const DataPath& outputArrayPath, bool markWatershedLine, bool fullyConnected, const std::atomic_bool& shouldCancel,
                                                             const IFilter::MessageHandler& messageHandler, MarkerIdConversionHandling conversionHandling)
{
  using TypeSetPolicyType = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::MorphologicalWatershedFromMarkersExecuteFn executeFn{dataStructure,     inputArrayPath, markerArrayPath, outputArrayPath, imageGeom.getDimensions(),
                                                               markWatershedLine, fullyConnected, shouldCancel,    messageHandler,  conversionHandling};
  return TypeSetPolicyType::Dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteMorphologicalWatershedImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, float64 level,
                                                  bool markWatershedLine, bool fullyConnected, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicyType = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::MorphologicalWatershedExecuteFn executeFn{.data_structure = dataStructure,
                                                    .input_array_path = inputArrayPath,
                                                    .output_array_path = outputArrayPath,
                                                    .dims = imageGeom.getDimensions(),
                                                    .level = level,
                                                    .mark_watershed_line = markWatershedLine,
                                                    .fully_connected = fullyConnected,
                                                    .should_cancel = shouldCancel,
                                                    .message_handler = messageHandler};
  return TypeSetPolicyType::Dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
