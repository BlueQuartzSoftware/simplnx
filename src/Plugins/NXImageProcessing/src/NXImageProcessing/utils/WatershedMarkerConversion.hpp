#pragma once

#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageTransferResult.hpp"

#include <algorithm>
#include <atomic>
#include <fmt/format.h>
#include <limits>
#include <new>
#include <nonstd/span.hpp>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing::detail
{
inline constexpr usize k_MarkerChunkValues = 65536;
inline constexpr int32 k_MarkerIdConversionError = -79073;
inline constexpr int32 k_MarkerIdConversionWarning = -79074;
inline constexpr int32 k_MarkerIdCopyAllocationFailed = -79075;
inline constexpr int32 k_MarkerIdRestoreAllocationFailed = -79076;

/**
 * @class MarkerIdConversionState
 * @brief Counts changed marker occurrences and retains the first change for diagnostics.
 * @tparam T Native marker type used for exact integer comparisons.
 */
template <class T>
class MarkerIdConversionState
{
public:
  MarkerIdConversionState() = default;
  ~MarkerIdConversionState() noexcept = default;
  MarkerIdConversionState(const MarkerIdConversionState&) = delete;
  MarkerIdConversionState(MarkerIdConversionState&&) = delete;
  MarkerIdConversionState& operator=(const MarkerIdConversionState&) = delete;
  MarkerIdConversionState& operator=(MarkerIdConversionState&&) = delete;

  /**
   * @brief Compares one native marker with its restored value and records any loss.
   * @param index Flat index in the marker array.
   * @param nativeValue Marker before encoding.
   * @param encodedValue Label used by the flood.
   * @return True if the encoding changed the marker identity.
   */
  bool observe(usize index, T nativeValue, uint32 encodedValue) noexcept
  {
    const T restoredValue = static_cast<T>(encodedValue);
    if(restoredValue == nativeValue)
    {
      return false;
    }
    if(m_ChangedCount == 0)
    {
      m_FirstIndex = index;
      m_FirstNativeValue = nativeValue;
      m_FirstRestoredValue = restoredValue;
    }
    ++m_ChangedCount;
    return true;
  }

  /**
   * @brief Adds a single conversion diagnostic for the observed prefix.
   * @param markerPath Identifies the input in the diagnostic.
   * @param handling Selects an error or a warning for a changed label.
   * @param result Transfer diagnostics collected before completion, cancellation, or failure.
   * @return Transfer result with the conversion diagnostic when at least one label changed.
   */
  Result<> finish(const DataPath& markerPath, MarkerIdConversionHandling handling, Result<> result) const
  {
    if(m_ChangedCount == 0)
    {
      return result;
    }
    const auto context = fmt::format("Marker array '{}' has {} changed marker occurrence(s) observed. First change at flat index {}: native ID {} restores as {} after uint32 conversion. ",
                                     markerPath.toString(), m_ChangedCount, m_FirstIndex, m_FirstNativeValue, m_FirstRestoredValue);
    if(handling == MarkerIdConversionHandling::Error)
    {
      return MergeResults(std::move(result),
                          MakeErrorResult(k_MarkerIdConversionError,
                                          context + "Execution stopped before flooding. Use IDs that round-trip, or select 'Warn and Continue (Legacy Behavior)' under "
                                                    "'Marker ID Conversion Handling' only if you intentionally accept seed loss, renamed IDs, merged basins, and incorrect region measurements."));
    }
    return MergeResults(std::move(result), MakeWarningVoidResult(k_MarkerIdConversionWarning,
                                                                 context + "'Warn and Continue (Legacy Behavior)' retains the old conversion and flood. Seeds can disappear or merge, IDs can be "
                                                                           "renamed, and basin boundaries, region sizes, and downstream feature measurements can be incorrect."));
  }

private:
  usize m_ChangedCount = 0;
  usize m_FirstIndex = 0;
  T m_FirstNativeValue = {};
  T m_FirstRestoredValue = {};
};

/**
 * @brief Encodes scalar native markers into uint32 flood labels in bounded batches.
 * @param dataStructure Contains the validated native marker array.
 * @param srcPath Selects the marker array and identifies it in diagnostics.
 * @param dst Receives the labels. The caller supplies a distinct scratch store of the same size.
 * @param shouldCancel Requests a stop between batches. Completed batches remain written.
 * @param conversionHandling Error stops before writing the first lossy batch. Warn retains the converted labels.
 * @return Transfer diagnostics and any conversion diagnostic. Cancellation alone leaves the result valid.
 * @note All 8-, 16-, and 32-bit integer values round-trip. The lossless 64-bit domain is zero through UINT32_MAX.
 */
inline Result<> WidenIntegerArrayToUInt32(const DataStructure& dataStructure, const DataPath& srcPath, AbstractDataStore<uint32>& dst, const std::atomic_bool& shouldCancel,
                                          MarkerIdConversionHandling conversionHandling = MarkerIdConversionHandling::Error)
{
  const auto& srcArray = dataStructure.getDataRefAs<IDataArray>(srcPath);
  return IntegerOnly::Dispatch(srcArray.getDataType(), [&]<class S>() -> Result<> {
    const AbstractDataStore<S>& source = dataStructure.getDataRefAs<DataArray<S>>(srcPath).getDataStoreRef();
    const usize total = source.getSize();
    MarkerIdConversionState<S> conversionState;
    Result<> accumulated;
    try
    {
      std::vector<S> inputValues(std::min(k_MarkerChunkValues, total));
      std::vector<uint32> encodedValues(std::min(k_MarkerChunkValues, total));
      for(usize start = 0; start < total;)
      {
        if(shouldCancel)
        {
          return conversionState.finish(srcPath, conversionHandling, std::move(accumulated));
        }
        const usize count = std::min(k_MarkerChunkValues, total - start);
        AccumulateTransferResult(accumulated, source.copyIntoBuffer(start, nonstd::span<S>(inputValues.data(), count)));
        if(accumulated.invalid())
        {
          return conversionState.finish(srcPath, conversionHandling, std::move(accumulated));
        }
        for(usize index = 0; index < count; ++index)
        {
          const S nativeValue = inputValues[index];
          // Promote narrow signed IDs without changing the legacy conversion modulo 2^32.
          const uint32 encodedValue = static_cast<uint32>(+nativeValue);
          if(conversionState.observe(start + index, nativeValue, encodedValue) && conversionHandling == MarkerIdConversionHandling::Error)
          {
            // Stop before writing this chunk or entering either flood route.
            return conversionState.finish(srcPath, conversionHandling, std::move(accumulated));
          }
          encodedValues[index] = encodedValue;
        }
        AccumulateTransferResult(accumulated, dst.copyFromBuffer(start, nonstd::span<const uint32>(encodedValues.data(), count)));
        if(accumulated.invalid())
        {
          return conversionState.finish(srcPath, conversionHandling, std::move(accumulated));
        }
        // count <= total - start, so both the next offset and start + index remain in range.
        start += count;
      }
      return conversionState.finish(srcPath, conversionHandling, std::move(accumulated));
    } catch(const std::bad_alloc&)
    {
      return conversionState.finish(
          srcPath, conversionHandling,
          MergeResults(std::move(accumulated), MakeErrorResult(k_MarkerIdCopyAllocationFailed, "Morphological watershed could not allocate working storage while copying marker IDs.")));
    }
  });
}

/**
 * @brief Writes flooded labels in the native marker type. A zero label remains zero.
 * @param src Contains the flooded uint32 labels in a distinct scratch store.
 * @param dataStructure Contains the preflight-created output array.
 * @param dstPath Selects the output array of the same size as the scratch store.
 * @param shouldCancel Requests a stop between batches. Completed output batches remain written.
 * @return Transfer diagnostics, including warnings preceding failure or cancellation. Cancellation alone leaves the result valid.
 */
inline Result<> NarrowUInt32StoreToIntegerArray(const AbstractDataStore<uint32>& src, DataStructure& dataStructure, const DataPath& dstPath, const std::atomic_bool& shouldCancel)
{
  const auto& dstArray = dataStructure.getDataRefAs<IDataArray>(dstPath);
  return IntegerOnly::Dispatch(dstArray.getDataType(), [&]<class D>() -> Result<> {
    AbstractDataStore<D>& destination = dataStructure.getDataRefAs<DataArray<D>>(dstPath).getDataStoreRef();
    const usize total = src.getSize();
    Result<> accumulated;
    try
    {
      std::vector<uint32> encodedValues(std::min(k_MarkerChunkValues, total));
      std::vector<D> outputValues(std::min(k_MarkerChunkValues, total));
      for(usize start = 0; start < total;)
      {
        if(shouldCancel)
        {
          return accumulated;
        }
        const usize count = std::min(k_MarkerChunkValues, total - start);
        AccumulateTransferResult(accumulated, src.copyIntoBuffer(start, nonstd::span<uint32>(encodedValues.data(), count)));
        if(accumulated.invalid())
        {
          return accumulated;
        }
        for(usize index = 0; index < count; ++index)
        {
          outputValues[index] = static_cast<D>(encodedValues[index]);
        }
        AccumulateTransferResult(accumulated, destination.copyFromBuffer(start, nonstd::span<const D>(outputValues.data(), count)));
        if(accumulated.invalid())
        {
          return accumulated;
        }
        start += count; // count <= total - start, including the final short chunk.
      }
      return accumulated;
    } catch(const std::bad_alloc&)
    {
      return MergeResults(std::move(accumulated), MakeErrorResult(k_MarkerIdRestoreAllocationFailed, "Morphological watershed could not allocate working storage while restoring marker IDs."));
    }
  });
}

/**
 * @brief Encodes the native marker maximum for the out-of-bounds neighbor value.
 * @param markerType Integer element type of the marker array.
 * @return The native maximum converted to uint32. An in-bounds seed with this value remains valid.
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
} // namespace nx::core::ImageProcessing::detail
