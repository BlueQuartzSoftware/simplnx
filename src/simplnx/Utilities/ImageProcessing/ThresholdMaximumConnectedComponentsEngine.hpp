#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageProcessing/ConnectedComponentEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StreamingStatistics.hpp"
#include "simplnx/Utilities/ImageProcessing/TerminalImageConversion.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <new>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
inline constexpr int32 k_ThresholdMaxCCMismatchedOutput = -28612;
inline constexpr int32 k_ThresholdMaxCCEmptyBand = -28615;
inline constexpr int32 k_ThresholdMaxCCSearchStalled = -28616;
inline constexpr int32 k_ThresholdMaxCCScanAllocationFailed = -28617;
inline constexpr int32 k_ThresholdMaxCCScanTooLarge = -28618;
inline constexpr int32 k_ThresholdMaxCCOutputAllocationFailed = -28634;
inline constexpr int32 k_ThresholdMaxCCOutputTooLarge = -28635;
inline constexpr int32 k_ThresholdMaxCCAllocationFailed = -28636;
inline constexpr int32 k_ThresholdMaxCCBufferTooLarge = -28637;

/**
 * @struct ThresholdMaxCCOptions
 * @brief Controls the native threshold search. Byte targets affect staging, not segmentation.
 */
struct ThresholdMaxCCOptions
{
  uint32 MinimumObjectSize = 0;
  float64 UpperBoundary = 65536.0;
  uint8 InsideValue = 1;
  uint8 OutsideValue = 0;
  usize CountTarget2DBytes = detail::k_ConnectedComponent2DTargetBytes;
};

namespace detail
{
template <class T>
ArrayMinMax<T> ReduceThresholdMaxCCBlock(nonstd::span<const T> values, const std::atomic_bool& shouldCancel)
{
  ArrayMinMax<T> summary;
  for(usize index = 0; index < values.size(); ++index)
  {
    if((index % 65536) == 0 && shouldCancel)
    {
      break;
    }
    const T value = values[index];
    if constexpr(std::is_floating_point_v<T>)
    {
      if(!std::isfinite(value))
      {
        continue;
      }
    }
    summary.addValue(value);
  }
  return summary;
}

/**
 * @brief Scans finite native bounds without changing shared statistics or making a full-image temporary.
 * @tparam T Numeric input value type.
 * @param input Source values to scan.
 * @param shouldCancel Requests cancellation during the scan.
 * @return Native finite bounds and contributor count, or a transfer or allocation error.
 * A valid cancellation result can be incomplete. The caller must check shouldCancel before use.
 */
template <class T>
Result<ArrayMinMax<T>> ScanThresholdMaxCC(const AbstractDataStore<T>& input, const std::atomic_bool& shouldCancel)
{
  ArrayMinMax<T> summary;
  Result<> status;
  try
  {
    const usize total = input.getSize();
    if(total == 0)
    {
      return {{summary}};
    }
    constexpr usize blockValues = 65536;
    constexpr usize residentBatchBytes = 32ULL * 1024ULL * 1024ULL;
    const auto* residentPtr = dynamic_cast<const DataStore<T>*>(&input);
    const bool direct = input.getStoreType() == IDataStore::StoreType::InMemory && residentPtr != nullptr;
    StreamingScanBufferPlan plan;
    usize batchValues = std::min(total, residentBatchBytes / sizeof(T));
    usize availableBytes = batchValues * sizeof(T);
    if(!direct)
    {
      auto planResult = CreateStreamingScanBufferPlan(input);
      if(planResult.invalid())
      {
        return ConvertInvalidResult<ArrayMinMax<T>>(std::move(planResult));
      }
      plan = std::move(planResult.value());
      batchValues = std::min(total, plan.batchValues);
      availableBytes = batchValues * sizeof(T);
    }
    usize blockCapacity = 1 + ((batchValues - 1) / blockValues);
    if(blockCapacity <= 1 || blockCapacity > (availableBytes / sizeof(ArrayMinMax<T>)))
    {
      blockCapacity = 0;
    }
    else if(!direct)
    {
      // Charge the bounded summary table before selecting the owned input payload.
      const usize summaryBytes = blockCapacity * sizeof(ArrayMinMax<T>);
      if(availableBytes - summaryBytes < sizeof(T))
      {
        blockCapacity = 0;
      }
      else
      {
        batchValues = (availableBytes - summaryBytes) / sizeof(T);
        plan.reservation.shrinkTo((batchValues * sizeof(T)) + summaryBytes);
      }
    }
    std::vector<ArrayMinMax<T>> blocks(blockCapacity);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays) -- A full transfer overwrites this contiguous buffer, including for bool.
    auto valuesPtr = direct ? nullptr : std::make_unique_for_overwrite<T[]>(batchValues);
    for(usize start = 0; start < total;)
    {
      if(shouldCancel)
      {
        break;
      }
      const usize count = std::min(batchValues, total - start);
      if(!direct)
      {
        auto readResult = input.copyIntoBuffer(start, nonstd::span<T>(valuesPtr.get(), count));
        status = MergeResults(std::move(status), std::move(readResult));
        if(status.invalid())
        {
          return ConvertInvalidResult<ArrayMinMax<T>>(std::move(status));
        }
      }
      const nonstd::span<const T> values = direct ? residentPtr->createSpan().subspan(start, count) : nonstd::span<const T>(valuesPtr.get(), count);
      if(blockCapacity == 0)
      {
        summary.merge(ReduceThresholdMaxCCBlock(values, shouldCancel));
      }
      else
      {
        const usize blockCount = 1 + ((count - 1) / blockValues);
        ParallelDataAlgorithm algorithm;
        algorithm.setRange(0, blockCount);
        algorithm.execute([&](const Range& range) {
          for(usize block = range.min(); block < range.max(); ++block)
          {
            const usize offset = block * blockValues;
            blocks[block] = ReduceThresholdMaxCCBlock(values.subspan(offset, std::min(blockValues, count - offset)), shouldCancel);
          }
        });
        for(usize block = 0; block < blockCount; ++block)
        {
          summary.merge(blocks[block]);
        }
      }
      start += count;
    }
    return {{summary}, std::move(status.warnings())};
  } catch(const std::bad_alloc&)
  {
    return ConvertResultTo(MergeResults(std::move(status), MakeErrorResult(k_ThresholdMaxCCScanAllocationFailed,
                                                                           "Threshold Maximum Connected Components could not allocate its scan buffers. Reduce the data size or provide more memory.")),
                           std::move(summary));
  } catch(const std::length_error&)
  {
    return ConvertResultTo(
        MergeResults(std::move(status), MakeErrorResult(k_ThresholdMaxCCScanTooLarge, "Threshold Maximum Connected Components exceeded an addressable scan buffer size. Reduce the image dimensions.")),
        std::move(summary));
  }
}

template <class T>
struct ThresholdMaxCCCap
{
  T Value{};
  bool Empty = false;
};

/**
 * @brief Converts an Upper Boundary request to a native finite cap or an empty-band result.
 * @tparam T Numeric input value type used for the cap.
 * @param value Upper Boundary request. Positive infinity selects the native maximum; negative infinity requests an empty band.
 * @return Native cap with Empty false, or Empty true for negative infinity or a request below the native lower bound.
 * @pre The caller has rejected NaN.
 *
 * For integer input types, finite requests truncate before the lower-bound check. Values above the native maximum use the native maximum.
 */
template <class T>
ThresholdMaxCCCap<T> ConvertThresholdMaxCCCap(float64 value)
{
  if(value == -std::numeric_limits<float64>::infinity())
  {
    return {T{}, true};
  }
  if(value == std::numeric_limits<float64>::infinity())
  {
    return {std::numeric_limits<T>::max(), false};
  }
  if constexpr(std::is_integral_v<T>)
  {
    if(std::trunc(value) < static_cast<float64>(std::numeric_limits<T>::lowest()))
    {
      return {T{}, true};
    }
    T converted{};
    // The shared pure conversion checks exact exclusive 2^63/2^64 before any integer cast.
    ConvertTerminalImageValue(value, converted);
    return {converted, false};
  }
  else
  {
    if(value < static_cast<float64>(std::numeric_limits<T>::lowest()))
    {
      return {T{}, true};
    }
    return {value >= static_cast<float64>(std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : NarrowFloatingImageValue<T>(value), false};
  }
}

template <class T>
bool ThresholdMaxCCSpanExceedsTwo(T low, T high)
{
  if constexpr(std::is_integral_v<T>)
  {
    using UnsignedType = std::make_unsigned_t<T>;
    const UnsignedType distance = static_cast<UnsignedType>(static_cast<UnsignedType>(high) - static_cast<UnsignedType>(low));
    return distance > UnsignedType{2};
  }
  else
  {
    // Same-sign subtraction cannot overflow. Opposite large signs already prove width > 2.
    if(low < T{0} && high > T{0} && (low < T{-2} || high > T{2}))
    {
      return true;
    }
    return (high - low) > T{2};
  }
}

template <class T>
struct ThresholdMaxCCSearchInterval
{
  T Low;
  T High;
  T FixedCap;
};

template <class T>
Result<T> SearchThresholdMaxCC(const AbstractDataStore<T>& input, const SizeVec3& dims, ThresholdMaxCCSearchInterval<T> interval, const ThresholdMaxCCOptions& options,
                               const std::atomic_bool& shouldCancel)
{
  auto [low, high, fixedCap] = interval;
  T midpoint = std::midpoint(low, high);
  T left = std::midpoint(low, midpoint);
  T right = std::midpoint(high, midpoint);
  Result<> status;
  // Score each candidate using the nonzero pixels it would write. Nonfinite pixels receive
  // Outside Value. They can connect foreground regions when Outside Value is nonzero.
  while(ThresholdMaxCCSpanExceedsTwo(low, high) && !shouldCancel)
  {
    const T oldLow = low;
    const T oldHigh = high;
    const T oldMidpoint = midpoint;
    const auto rightPredicate = [&](T value) { return (value >= right && value <= fixedCap) ? options.InsideValue != 0 : options.OutsideValue != 0; };
    auto rightResult = CountConnectedComponents(input, dims, rightPredicate, false, options.MinimumObjectSize, shouldCancel, options.CountTarget2DBytes);
    if(rightResult.invalid())
    {
      return ConvertResultTo(MergeResults(std::move(status), ConvertResult(std::move(rightResult))), T{midpoint});
    }
    const uint32 rightCount = rightResult.value();
    status = MergeResults(std::move(status), ConvertResult(std::move(rightResult)));
    if(shouldCancel)
    {
      break;
    }
    const auto leftPredicate = [&](T value) { return (value >= left && value <= fixedCap) ? options.InsideValue != 0 : options.OutsideValue != 0; };
    auto leftResult = CountConnectedComponents(input, dims, leftPredicate, false, options.MinimumObjectSize, shouldCancel, options.CountTarget2DBytes);
    if(leftResult.invalid())
    {
      return ConvertResultTo(MergeResults(std::move(status), ConvertResult(std::move(leftResult))), T{midpoint});
    }
    const uint32 leftCount = leftResult.value();
    status = MergeResults(std::move(status), ConvertResult(std::move(leftResult)));
    if(shouldCancel)
    {
      break;
    }
    if(rightCount > leftCount)
    {
      low = midpoint;
      midpoint = right;
    }
    else
    {
      high = midpoint;
      midpoint = left;
    }
    left = std::midpoint(low, midpoint);
    right = std::midpoint(high, midpoint);
    if constexpr(std::is_floating_point_v<T>)
    {
      if(low == oldLow && high == oldHigh && midpoint == oldMidpoint && ThresholdMaxCCSpanExceedsTwo(low, high))
      {
        status.warnings().push_back(Warning{.code = k_ThresholdMaxCCSearchStalled,
                                            .message = "Threshold search stopped because native floating precision repeated the complete search state while the interval was wider than "
                                                       "2 intensity units. The candidate selected by the normal component-count comparison was retained; segmentation is heuristic."});
        break;
      }
    }
  }
  return ConvertResultTo(std::move(status), std::move(midpoint));
}

template <class T>
Result<> WriteThresholdMaxCC(const AbstractDataStore<T>& input, AbstractDataStore<uint8>& output, bool fillOutside, T threshold, T cap, const ThresholdMaxCCOptions& options,
                             const std::atomic_bool& shouldCancel)
{
  Result<> status;
  try
  {
    const usize total = input.getSize();
    constexpr usize minimumValues = 65536;
    constexpr usize preferredBytes = 32ULL * 1024ULL * 1024ULL;
    const usize valueBytes = fillOutside ? sizeof(uint8) : sizeof(T) + sizeof(uint8);
    const uint64 usefulBytes = total > std::numeric_limits<usize>::max() / valueBytes ? std::numeric_limits<uint64>::max() : static_cast<uint64>(total) * valueBytes;
    auto reservation = ReserveWorkingMemory(preferredBytes, usefulBytes);
    const usize chunkValues = std::max<usize>(minimumValues, std::min(total, static_cast<usize>(std::min<uint64>(reservation.sizeBytes() / valueBytes, std::numeric_limits<usize>::max()))));
    reservation.shrinkTo(static_cast<uint64>(std::min(chunkValues, total)) * valueBytes);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays) -- A full transfer overwrites this contiguous buffer, including for bool.
    auto inputPtr = fillOutside ? nullptr : std::make_unique_for_overwrite<T[]>(std::min(chunkValues, total));
    auto outputPtr = std::make_unique<std::vector<uint8>>(std::min(chunkValues, total));
    for(usize start = 0; start < total;)
    {
      if(shouldCancel)
      {
        break;
      }
      const usize count = std::min(chunkValues, total - start);
      if(!fillOutside)
      {
        auto readResult = input.copyIntoBuffer(start, nonstd::span<T>(inputPtr.get(), count));
        status = MergeResults(std::move(status), std::move(readResult));
        if(status.invalid() || shouldCancel)
        {
          return status;
        }
      }
      for(usize index = 0; index < count; ++index)
      {
        (*outputPtr)[index] = !fillOutside && inputPtr[index] >= threshold && inputPtr[index] <= cap ? options.InsideValue : options.OutsideValue;
      }
      auto writeResult = output.copyFromBuffer(start, nonstd::span<const uint8>(outputPtr->data(), count));
      status = MergeResults(std::move(status), std::move(writeResult));
      if(status.invalid())
      {
        return status;
      }
      start += count;
    }
    return status;
  } catch(const std::bad_alloc&)
  {
    return MergeResults(std::move(status), MakeErrorResult(k_ThresholdMaxCCOutputAllocationFailed,
                                                           "Threshold Maximum Connected Components could not allocate its output buffers. Reduce the data size or provide more memory."));
  } catch(const std::length_error&)
  {
    return MergeResults(std::move(status),
                        MakeErrorResult(k_ThresholdMaxCCOutputTooLarge, "Threshold Maximum Connected Components exceeded an addressable output buffer size. Reduce the image dimensions."));
  }
}
} // namespace detail

/**
 * @brief Searches native finite bounds using the nonzero-output component-count heuristic and absolute two-unit stop.
 * @tparam T Numeric input value type.
 * @param input Scalar source image in X-fastest order.
 * @param output Scalar uint8 destination with the input value count.
 * @param dims Image dimensions in X, Y, Z order.
 * @param options Component-size, cutoff, output-value and staging controls.
 * @param shouldCancel Requests cancellation during scan, search, and output work.
 * @param messageHandler Receives output-phase progress messages.
 * @return Collected warnings, or a parameter, shape, numeric-capacity, allocation, or transfer error.
 * Cancellation retains the valid engine-result convention.
 *
 * @pre The caller has rejected a NaN Upper Boundary.
 *
 * Finite values define the search bounds. When none contribute, every output pixel receives Outside Value.
 * Count capacity remains strict. Owned scan and output buffers are bounded; component-run tables depend on image topology.
 * Completed output is not rolled back after a late failure or cancellation.
 */
template <class T>
Result<> ApplyThresholdMaximumConnectedComponents(const AbstractDataStore<T>& input, AbstractDataStore<uint8>& output, const SizeVec3& dims, const ThresholdMaxCCOptions& options,
                                                  const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler = {})
{
  Result<> status;
  if(input.getNumberOfComponents() != 1 || output.getNumberOfComponents() != 1 || output.getSize() != input.getSize())
  {
    return MakeErrorResult(k_ThresholdMaxCCMismatchedOutput,
                           "Threshold Maximum Connected Components requires scalar input and uint8 output with matching tuple counts. Recreate the output for the selected image.");
  }
  usize sliceValues = 0;
  usize volumeValues = 0;
  if(auto validation = detail::ValidateConnectedComponentVolume(input, dims, sliceValues, volumeValues); validation.invalid())
  {
    return validation;
  }
  try
  {
    auto summaryResult = detail::ScanThresholdMaxCC(input, shouldCancel);
    if(summaryResult.invalid())
    {
      return ConvertResult(std::move(summaryResult));
    }
    const auto summary = summaryResult.value();
    status = ConvertResult(std::move(summaryResult));
    if(shouldCancel || input.getSize() == 0)
    {
      return status;
    }
    const auto cap = detail::ConvertThresholdMaxCCCap<T>(options.UpperBoundary);
    const bool noFinite = summary.count == 0;
    const bool emptyBand = cap.Empty || (!noFinite && cap.Value < summary.min);
    if(emptyBand)
    {
      status.warnings().push_back(Warning{.code = k_ThresholdMaxCCEmptyBand,
                                          .message = fmt::format("Upper Boundary requests an empty native intensity band. Threshold search was skipped; every output pixel is set to Outside Value "
                                                                 "({}). A nonzero Outside Value can make the whole image foreground downstream.",
                                                                 options.OutsideValue)});
    }
    if(noFinite || emptyBand)
    {
      auto writeResult = detail::WriteThresholdMaxCC(input, output, true, T{}, T{}, options, shouldCancel);
      return MergeResults(std::move(status), std::move(writeResult));
    }
    auto thresholdResult = detail::SearchThresholdMaxCC(input, dims, {.Low = summary.min, .High = std::min(summary.max, cap.Value), .FixedCap = cap.Value}, options, shouldCancel);
    if(thresholdResult.invalid())
    {
      return MergeResults(std::move(status), ConvertResult(std::move(thresholdResult)));
    }
    const T threshold = thresholdResult.value();
    status = MergeResults(std::move(status), ConvertResult(std::move(thresholdResult)));
    if(shouldCancel)
    {
      return status;
    }
    messageHandler.sendInfoMessage("Applying the selected native threshold in bounded batches");
    auto writeResult = detail::WriteThresholdMaxCC(input, output, false, threshold, cap.Value, options, shouldCancel);
    return MergeResults(std::move(status), std::move(writeResult));
  } catch(const std::bad_alloc&)
  {
    return MergeResults(std::move(status), MakeErrorResult(k_ThresholdMaxCCAllocationFailed,
                                                           "Threshold Maximum Connected Components could not allocate its scan or output buffers. Reduce the data size or provide more memory."));
  } catch(const std::length_error&)
  {
    return MergeResults(std::move(status), MakeErrorResult(k_ThresholdMaxCCBufferTooLarge, "Threshold Maximum Connected Components exceeded an addressable buffer size. Reduce the image dimensions."));
  }
}
} // namespace nx::core::ImageProcessing
