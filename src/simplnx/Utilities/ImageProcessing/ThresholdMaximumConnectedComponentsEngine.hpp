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
  bool WarnOnNonfinite = false;
  usize CountTarget2DBytes = detail::k_ConnectedComponent2DTargetBytes;
};

namespace detail
{
template <class T>
struct ThresholdMaxCCSummary
{
  T Minimum{};
  T Maximum{};
  usize FiniteCount = 0;
  usize NaNCount = 0;
  usize NegativeInfinityCount = 0;
  usize PositiveInfinityCount = 0;
};

template <class T>
void MergeThresholdMaxCCSummary(ThresholdMaxCCSummary<T>& summary, const ThresholdMaxCCSummary<T>& block)
{
  if(block.FiniteCount != 0)
  {
    if(summary.FiniteCount == 0)
    {
      summary.Minimum = block.Minimum;
      summary.Maximum = block.Maximum;
    }
    else
    {
      summary.Minimum = std::min(summary.Minimum, block.Minimum);
      summary.Maximum = std::max(summary.Maximum, block.Maximum);
    }
  }
  summary.FiniteCount += block.FiniteCount;
  summary.NaNCount += block.NaNCount;
  summary.NegativeInfinityCount += block.NegativeInfinityCount;
  summary.PositiveInfinityCount += block.PositiveInfinityCount;
}

template <class T>
ThresholdMaxCCSummary<T> ReduceThresholdMaxCCBlock(nonstd::span<const T> values, bool warnOnNonfinite, const std::atomic_bool& shouldCancel)
{
  ThresholdMaxCCSummary<T> summary;
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
        if(std::isnan(value))
        {
          ++summary.NaNCount;
        }
        else if(std::signbit(value))
        {
          ++summary.NegativeInfinityCount;
        }
        else
        {
          ++summary.PositiveInfinityCount;
        }
        if(!warnOnNonfinite)
        {
          break;
        }
        continue;
      }
    }
    if(summary.FiniteCount == 0)
    {
      summary.Minimum = value;
      summary.Maximum = value;
    }
    else
    {
      summary.Minimum = std::min(summary.Minimum, value);
      summary.Maximum = std::max(summary.Maximum, value);
    }
    ++summary.FiniteCount;
  }
  return summary;
}

/**
 * @brief Scans finite native bounds without changing shared statistics or making a full-image temporary.
 * @tparam T Numeric input value type.
 * @param input Source values to scan.
 * @param warnOnNonfinite If true, counts nonfinite samples and continues. If false, a nonfinite sample causes an error.
 * @param shouldCancel Requests cancellation during the scan.
 * @return Native finite bounds and sample counts, or a transfer or allocation error, or an error for disallowed nonfinite samples.
 * A valid cancellation result can be incomplete. The caller must check shouldCancel before use.
 */
template <class T>
Result<ThresholdMaxCCSummary<T>> ScanThresholdMaxCC(const AbstractDataStore<T>& input, bool warnOnNonfinite, const std::atomic_bool& shouldCancel)
{
  ThresholdMaxCCSummary<T> summary;
  Result<> status;
  try
  {
    const usize total = input.getSize();
    if(total == 0)
    {
      return {summary};
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
        return ConvertInvalidResult<ThresholdMaxCCSummary<T>>(std::move(planResult));
      }
      plan = std::move(planResult.value());
      batchValues = std::min(total, plan.batchValues);
      availableBytes = batchValues * sizeof(T);
    }
    usize blockCapacity = 1 + ((batchValues - 1) / blockValues);
    if(blockCapacity <= 1 || blockCapacity > (availableBytes / sizeof(ThresholdMaxCCSummary<T>)))
    {
      blockCapacity = 0;
    }
    else if(!direct)
    {
      // Charge the bounded summary table before selecting the owned input payload.
      const usize summaryBytes = blockCapacity * sizeof(ThresholdMaxCCSummary<T>);
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
    std::vector<ThresholdMaxCCSummary<T>> blocks(blockCapacity);
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
          return ConvertInvalidResult<ThresholdMaxCCSummary<T>>(std::move(status));
        }
      }
      const nonstd::span<const T> values = direct ? residentPtr->createSpan().subspan(start, count) : nonstd::span<const T>(valuesPtr.get(), count);
      if(blockCapacity == 0)
      {
        MergeThresholdMaxCCSummary(summary, ReduceThresholdMaxCCBlock(values, warnOnNonfinite, shouldCancel));
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
            blocks[block] = ReduceThresholdMaxCCBlock(values.subspan(offset, std::min(blockValues, count - offset)), warnOnNonfinite, shouldCancel);
          }
        });
        for(usize block = 0; block < blockCount; ++block)
        {
          MergeThresholdMaxCCSummary(summary, blocks[block]);
        }
      }
      if(!warnOnNonfinite && (summary.NaNCount != 0 || summary.NegativeInfinityCount != 0 || summary.PositiveInfinityCount != 0))
      {
        return ConvertResultTo(MergeResults(std::move(status), MakeErrorResult(-28611, "Threshold Maximum Connected Components found image NaN or infinity during the initial scan. Select Warn and "
                                                                                       "Continue under Nonfinite Input Handling only if Outside Value participation is intended.")),
                               std::move(summary));
      }
      start += count;
    }
    return {summary, std::move(status.warnings())};
  } catch(const std::bad_alloc&)
  {
    return ConvertResultTo(
        MergeResults(std::move(status), MakeErrorResult(-28617, "Threshold Maximum Connected Components could not allocate its scan buffers. Reduce the data size or provide more memory.")),
        std::move(summary));
  } catch(const std::length_error&)
  {
    return ConvertResultTo(MergeResults(std::move(status), MakeErrorResult(-28618, "Threshold Maximum Connected Components exceeded an addressable scan buffer size. Reduce the image dimensions.")),
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
Result<T> SearchThresholdMaxCC(const AbstractDataStore<T>& input, const SizeVec3& dims, T low, T high, T fixedCap, const ThresholdMaxCCOptions& options, const std::atomic_bool& shouldCancel)
{
  T midpoint = std::midpoint(low, high);
  T left = std::midpoint(low, midpoint);
  T right = std::midpoint(high, midpoint);
  Result<> status;
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
        status.warnings().push_back(Warning{.code = -28616,
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
    auto inputPtr = fillOutside ? nullptr : std::make_unique_for_overwrite<T[]>(std::min(chunkValues, total));
    auto outputPtr = std::make_unique_for_overwrite<uint8[]>(std::min(chunkValues, total));
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
        outputPtr[index] = !fillOutside && inputPtr[index] >= threshold && inputPtr[index] <= cap ? options.InsideValue : options.OutsideValue;
      }
      auto writeResult = output.copyFromBuffer(start, nonstd::span<const uint8>(outputPtr.get(), count));
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
    return MergeResults(std::move(status), MakeErrorResult(-28617, "Threshold Maximum Connected Components could not allocate its output buffers. Reduce the data size or provide more memory."));
  } catch(const std::length_error&)
  {
    return MergeResults(std::move(status), MakeErrorResult(-28618, "Threshold Maximum Connected Components exceeded an addressable output buffer size. Reduce the image dimensions."));
  }
}
} // namespace detail

/**
 * @brief Searches native finite bounds using the nonzero-output component-count heuristic and absolute two-unit stop.
 * @tparam T Numeric input value type.
 * @param input Scalar source image in X-fastest order.
 * @param output Scalar uint8 destination with the input value count.
 * @param dims Image dimensions in X, Y, Z order.
 * @param options Component-size, cutoff, output-value, nonfinite, and staging controls.
 * @param shouldCancel Requests cancellation during scan, search, and output work.
 * @param messageHandler Receives output-phase progress messages.
 * @return Collected warnings, or a parameter, shape, numeric-capacity, allocation, or transfer error.
 * Cancellation retains the valid engine-result convention.
 *
 * NaN Upper Boundary always fails. Image Error fails in the initial scan, including for an empty requested band.
 * Warn retains ordinary inclusive comparisons. With no finite data, the engine fills Outside Value and reports that no threshold was calculated.
 * Count capacity remains strict. Owned scan and output buffers are bounded; component-run tables depend on image topology.
 * Completed output is not rolled back after a late failure or cancellation.
 */
template <class T>
Result<> ApplyThresholdMaximumConnectedComponents(const AbstractDataStore<T>& input, AbstractDataStore<uint8>& output, const SizeVec3& dims, const ThresholdMaxCCOptions& options,
                                                  const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler = {})
{
  Result<> status;
  if(std::isnan(options.UpperBoundary))
  {
    return MakeErrorResult(-28610, "Upper Boundary is NaN. Enter a valid numeric cutoff; positive infinity selects the native maximum and negative infinity requests an empty band.");
  }
  if(input.getNumberOfComponents() != 1 || output.getNumberOfComponents() != 1 || output.getSize() != input.getSize())
  {
    return MakeErrorResult(-28612, "Threshold Maximum Connected Components requires scalar input and uint8 output with matching tuple counts. Recreate the output for the selected image.");
  }
  usize sliceValues = 0;
  usize volumeValues = 0;
  if(auto validation = detail::ValidateConnectedComponentVolume(input, dims, sliceValues, volumeValues); validation.invalid())
  {
    return validation;
  }
  try
  {
    auto summaryResult = detail::ScanThresholdMaxCC(input, options.WarnOnNonfinite, shouldCancel);
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
    const bool noFinite = summary.FiniteCount == 0;
    const bool emptyBand = cap.Empty || (!noFinite && cap.Value < summary.Minimum);
    if(summary.NaNCount != 0 || summary.NegativeInfinityCount != 0 || summary.PositiveInfinityCount != 0)
    {
      status.warnings().push_back(Warning{
          .code = -28614,
          .message =
              fmt::format("Input contains {} NaN, {} negative infinity and {} positive infinity pixels. {} Outside Value is {}. Nonzero Outside pixels remain foreground and can connect regions.",
                          summary.NaNCount, summary.NegativeInfinityCount, summary.PositiveInfinityCount,
                          noFinite ? "No finite pixels are available; threshold search was skipped and no threshold was calculated. Every output pixel is set to Outside Value." :
                                     "Finite pixels define the search range; nonfinite pixels receive Outside Value through ordinary comparisons.",
                          options.OutsideValue)});
    }
    if(emptyBand && !noFinite)
    {
      status.warnings().push_back(Warning{.code = -28615,
                                          .message = fmt::format("Upper Boundary requests an empty native intensity band. Threshold search was skipped; every output pixel is set to Outside Value "
                                                                 "({}). A nonzero Outside Value can make the whole image foreground downstream.",
                                                                 options.OutsideValue)});
    }
    if(noFinite || emptyBand)
    {
      auto writeResult = detail::WriteThresholdMaxCC(input, output, true, T{}, T{}, options, shouldCancel);
      return MergeResults(std::move(status), std::move(writeResult));
    }
    auto thresholdResult = detail::SearchThresholdMaxCC(input, dims, summary.Minimum, std::min(summary.Maximum, cap.Value), cap.Value, options, shouldCancel);
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
    return MergeResults(std::move(status),
                        MakeErrorResult(-28617, "Threshold Maximum Connected Components could not allocate its scan or output buffers. Reduce the data size or provide more memory."));
  } catch(const std::length_error&)
  {
    return MergeResults(std::move(status), MakeErrorResult(-28618, "Threshold Maximum Connected Components exceeded an addressable buffer size. Reduce the image dimensions."));
  }
}
} // namespace nx::core::ImageProcessing
