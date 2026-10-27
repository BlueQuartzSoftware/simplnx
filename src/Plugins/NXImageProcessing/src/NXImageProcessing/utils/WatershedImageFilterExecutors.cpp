#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "NXImageProcessing/utils/ImageFilterExecutorDetail.hpp"

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
 * @brief Dispatched execute body for MorphologicalWatershedFromMarkers. Dispatches on the GRAYSCALE element type T
 *        (the FAH key). Every endpoint copies native markers into uint32 scratch, floods using the existing route,
 *        and converts the result back to the marker type. Copy, flood, and restore diagnostics are retained together.
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
  MarkerIdConversionHandling ConversionHandling = MarkerIdConversionHandling::Error;

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

      accumulated = WidenIntegerArrayToUInt32(dataStructure, markerArrayPath, *markerU32, shouldCancel, ConversionHandling);
      if(accumulated.invalid())
      {
        return accumulated;
      }
      const uint32 borderSentinel = MarkerTypeMaxAsUInt32(markerArray.getDataType());
      auto floodResult = ApplyWatershedFromMarkers<T>(inStore, *markerU32, *outU32, dims, markWatershedLine, fullyConnected, borderSentinel, shouldCancel, messageHandler);
      accumulated = MergeResults(std::move(accumulated), std::move(floodResult));
      if(accumulated.invalid())
      {
        return accumulated;
      }
      auto restoreResult = NarrowUInt32StoreToIntegerArray(*outU32, dataStructure, outputArrayPath, shouldCancel);
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

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteMorphologicalWatershedFromMarkersImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& markerArrayPath,
                                                             const DataPath& outputArrayPath, bool markWatershedLine, bool fullyConnected, const std::atomic_bool& shouldCancel,
                                                             const IFilter::MessageHandler& messageHandler, MarkerIdConversionHandling conversionHandling)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::MorphologicalWatershedFromMarkersExecuteFn executeFn{dataStructure,     inputArrayPath, markerArrayPath, outputArrayPath, imageGeom.getDimensions(),
                                                               markWatershedLine, fullyConnected, shouldCancel,    messageHandler,  conversionHandling};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteMorphologicalWatershedImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, float64 level,
                                                  bool markWatershedLine, bool fullyConnected, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  detail::MorphologicalWatershedExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, imageGeom.getDimensions(), level, markWatershedLine, fullyConnected, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
