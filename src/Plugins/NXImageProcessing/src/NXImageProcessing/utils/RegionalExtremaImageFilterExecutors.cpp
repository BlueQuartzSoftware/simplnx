#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "NXImageProcessing/utils/ImageFilterExecutorDetail.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/RegionalExtremaEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StreamingStatistics.hpp"

#include <algorithm>
#include <atomic>
#include <limits>
#include <memory>
#include <nonstd/expected.hpp>
#include <nonstd/span.hpp>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
namespace
{
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

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteValuedRegionalExtremaImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, RegionalExtremaOp op,
                                                 bool fullyConnected, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::ValuedRegionalExtremaExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, op, fullyConnected, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteRegionalExtremaImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, RegionalExtremaOp op,
                                           bool fullyConnected, float64 foreground, float64 background, bool flatIsExtremum, const std::atomic_bool& shouldCancel,
                                           const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  detail::RegionalExtremaExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, op, fullyConnected, foreground, background, flatIsExtremum, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
