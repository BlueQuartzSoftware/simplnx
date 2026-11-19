#pragma once

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/HistogramEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/PointwiseEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StreamingStatistics.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
inline constexpr int32 k_OtsuTooFewBins = -28900;
inline constexpr int32 k_OtsuInvalidThresholdCount = -28901;
inline constexpr int32 k_OtsuInvalidOutputShape = -28902;
inline constexpr int32 k_OtsuBinAllocationTooLarge = -28904;
inline constexpr int32 k_OtsuEmptyHistogram = -28905;
inline constexpr int32 k_OtsuInvalidHistogram = -28906;
inline constexpr int32 k_OtsuNonScalarStore = -28910;
inline constexpr int32 k_OtsuInvalidBinWidth = -28956;
inline constexpr int32 k_OtsuCollapsedBins = -28957;
inline constexpr int32 k_OtsuInvalidBinCoordinate = -28958;

/**
 * @struct OtsuImageOptions
 * @brief Holds Otsu controls for fixed uint8 output labels.
 */
struct OtsuImageOptions
{
  uint32 NumberOfBins = 128;
  uint8 NumberOfThresholds = 1;
  uint8 LabelOffset = 0;
  uint8 ExcludedLabel = 255;
  bool ValleyEmphasis = false;
  bool ReturnBinMidpoint = false;
  usize TargetBytes = 64ULL * 1024ULL * 1024ULL;
};

/**
 * @struct OtsuHistogramResult
 * @brief Holds the selected histogram and sample counts. Excluded samples are absent from every count.
 */
struct OtsuHistogramResult
{
  Histogram HistogramData;
  usize FiniteCount = 0;
};

namespace detail
{
// Match the existing floating statistics reduction grain; batch caps also bound the summary table.
inline constexpr usize k_OtsuRangeBlockValues = 262144;
inline constexpr usize k_OtsuMaximumBatchBytes = 64ULL * 1024ULL * 1024ULL;

// Avoid scheduling workers for short masked labeling batches. A one-threshold benchmark suggested an initial cutoff of 16,384 pixels.
// In Release timings with 1, 2, 4 and 8 thresholds on Intel and AMD CPUs, 512 was the largest tested power of two that reduced both median
// classification time and median whole-engine time at every threshold count. At 768 pixels, serial classification was slower with eight thresholds.
inline constexpr usize k_OtsuMaxSerialLabelPixels = 512;

template <class T, class MaskT>
ArrayMinMax<T> ReduceOtsuFloatingBlock(nonstd::span<const T> values, nonstd::span<const MaskT> maskValues, const std::atomic_bool& shouldCancel)
{
  ArrayMinMax<T> summary;
  for(usize index = 0; index < values.size(); ++index)
  {
    if((index % 65536) == 0 && shouldCancel)
    {
      return summary;
    }
    if(!maskValues.empty() && !maskValues[index])
    {
      continue;
    }
    const T value = values[index];
    if(std::isfinite(value))
    {
      summary.addValue(value);
    }
  }
  return summary;
}

template <class T, class MaskT>
ArrayMinMax<T> ReduceOtsuFloatingRange(nonstd::span<const T> values, nonstd::span<const MaskT> maskValues, const std::atomic_bool& shouldCancel)
{
  if(values.size() <= k_OtsuRangeBlockValues)
  {
    return ReduceOtsuFloatingBlock(values, maskValues, shouldCancel);
  }
  // The caller caps each resident view/owned batch. This table never has one entry per full-image block.
  const usize blockCount = 1 + ((values.size() - 1) / k_OtsuRangeBlockValues);
  std::vector<ArrayMinMax<T>> summaries(blockCount);
  ParallelDataAlgorithm algorithm;
  algorithm.setRange(0, blockCount);
  algorithm.execute([&](const Range& range) {
    for(usize block = range.min(); block < range.max(); ++block)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize start = block * k_OtsuRangeBlockValues;
      const usize count = std::min(k_OtsuRangeBlockValues, values.size() - start);
      summaries[block] = ReduceOtsuFloatingBlock(values.subspan(start, count), maskValues.empty() ? nonstd::span<const MaskT>{} : maskValues.subspan(start, count), shouldCancel);
    }
  });
  ArrayMinMax<T> summary;
  for(const auto& block : summaries)
  {
    if(shouldCancel)
    {
      return {};
    }
    summary.merge(block);
  }
  return summary;
}

struct OtsuBufferPlan
{
  CacheMemoryBudgetManager::WorkingMemoryReservation Reservation;
  usize Values = 1;
};

struct OtsuBufferRequirements
{
  usize TotalValues = 0;
  usize BytesPerValue = 1;
  bool UsesExternalStore = false;
  usize SummaryBytesPerBlock = 0;
};

inline OtsuBufferPlan ReserveOtsuBuffers(const OtsuImageOptions& options, const OtsuBufferRequirements& requirements)
{
  const auto& [total, bytesPerValue, usesExternalStore, summaryBytesPerBlock] = requirements;
  OtsuBufferPlan plan;
  const usize usefulBytes = total > std::numeric_limits<usize>::max() / bytesPerValue ? std::numeric_limits<usize>::max() : total * bytesPerValue;
  usize bytes = std::min({options.TargetBytes, k_OtsuMaximumBatchBytes, usefulBytes});
  if(usesExternalStore)
  {
    plan.Reservation = ReserveWorkingMemory(bytes, usefulBytes);
    bytes = static_cast<usize>(std::min<uint64>(plan.Reservation.sizeBytes(), std::numeric_limits<usize>::max()));
  }
  // One tuple is the bounded fallback when the aggregate grant cannot hold a complete tuple.
  plan.Values = std::max<usize>(1, std::min(total, bytes / bytesPerValue));
  if(summaryBytesPerBlock != 0 && plan.Values > k_OtsuRangeBlockValues)
  {
    // Charge the preliminary summary table before choosing payload size. A smaller payload cannot need more summaries.
    const usize summaryCount = 1 + ((plan.Values - 1) / k_OtsuRangeBlockValues);
    const usize summaryBytes = summaryCount * summaryBytesPerBlock;
    plan.Values = std::max<usize>(1, std::min(total, (bytes - summaryBytes) / bytesPerValue));
  }
  const usize summaryCount = summaryBytesPerBlock != 0 && plan.Values > k_OtsuRangeBlockValues ? 1 + ((plan.Values - 1) / k_OtsuRangeBlockValues) : 0;
  plan.Reservation.shrinkTo((plan.Values * bytesPerValue) + (summaryCount * summaryBytesPerBlock));
  return plan;
}

template <class T, class MaskT, class VisitorT>
Result<> VisitOtsuValues(const AbstractDataStore<T>& input, const AbstractDataStore<MaskT>* maskPtr, const OtsuImageOptions& options, const std::atomic_bool& shouldCancel, usize batchValues,
                         const VisitorT& visitor, bool parallelRange = false)
{
  Result<> status;
  const usize total = input.getSize();
  const auto* inputPtr = dynamic_cast<const DataStore<T>*>(&input);
  const auto* residentMaskPtr = maskPtr == nullptr ? nullptr : dynamic_cast<const DataStore<MaskT>*>(maskPtr);
  const bool direct = input.getStoreType() == IDataStore::StoreType::InMemory && inputPtr != nullptr &&
                      (maskPtr == nullptr || (maskPtr->getStoreType() == IDataStore::StoreType::InMemory && residentMaskPtr != nullptr));
  if(direct)
  {
    const auto values = inputPtr->createSpan();
    const auto maskValues = residentMaskPtr == nullptr ? nonstd::span<const MaskT>{} : residentMaskPtr->createSpan();
    const usize blockValues = parallelRange ? std::max<usize>(1, std::min(options.TargetBytes, k_OtsuMaximumBatchBytes) / (sizeof(T) + (maskPtr == nullptr ? 0 : sizeof(MaskT)))) : 65536;
    for(usize start = 0; start < total;)
    {
      if(shouldCancel)
      {
        return status;
      }
      const usize count = std::min(blockValues, total - start);
      status = MergeResults(std::move(status), visitor(values.subspan(start, count), maskPtr == nullptr ? nonstd::span<const MaskT>{} : maskValues.subspan(start, count), start));
      if(status.invalid())
      {
        return status;
      }
      start += count;
    }
    return status;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays) -- A full transfer overwrites this contiguous buffer, including for bool.
  auto valuesPtr = std::make_unique_for_overwrite<T[]>(batchValues);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays) -- A full mask transfer overwrites the contiguous bool or uint8 buffer.
  auto maskValuesPtr = maskPtr == nullptr ? nullptr : std::make_unique_for_overwrite<MaskT[]>(batchValues);
  for(usize start = 0; start < total;)
  {
    if(shouldCancel)
    {
      return status;
    }
    const usize count = std::min(batchValues, total - start);
    if(maskPtr != nullptr)
    {
      Result<> readMaskResult = maskPtr->copyIntoBuffer(start, nonstd::span<MaskT>(maskValuesPtr.get(), count));
      status = MergeResults(std::move(status), std::move(readMaskResult));
      if(status.invalid())
      {
        return status;
      }
    }
    if(shouldCancel)
    {
      return status;
    }
    Result<> readResult = input.copyIntoBuffer(start, nonstd::span<T>(valuesPtr.get(), count));
    status = MergeResults(std::move(status), std::move(readResult));
    if(status.invalid())
    {
      return status;
    }
    if(shouldCancel)
    {
      return status;
    }
    status = MergeResults(std::move(status),
                          visitor(nonstd::span<const T>(valuesPtr.get(), count), maskPtr == nullptr ? nonstd::span<const MaskT>{} : nonstd::span<const MaskT>(maskValuesPtr.get(), count), start));
    if(status.invalid())
    {
      return status;
    }
    start += count;
  }
  return status;
}

inline Result<> ConfigureCheckedOtsuBins(Histogram& histogram, bool nativeConstant, const std::atomic_bool& shouldCancel)
{
  if(nativeConstant)
  {
    histogram.binWidth = 0.0;
    return {};
  }
  const float64 range = histogram.max - histogram.min;
  const float64 numBins = static_cast<float64>(histogram.counts.size());
  const float64 upper = histogram.max + (range / (numBins * 100.0));
  histogram.binWidth = (upper - histogram.min) / numBins;
  if(!std::isfinite(range) || range <= 0.0 || !std::isfinite(upper) || upper <= histogram.min || !std::isfinite(histogram.binWidth) || histogram.binWidth <= 0.0 ||
     !std::isfinite(1.0 / histogram.binWidth))
  {
    return MakeErrorResult(k_OtsuInvalidBinWidth,
                           "Otsu cannot form a finite positive histogram range/bin width from the selected native values. Rescale the finite input or use fewer histogram bins.");
  }
  float64 previousBoundary = histogram.min;
  float64 previousMeasurement = histogram.min;
  for(usize bin = 0; bin < histogram.counts.size(); ++bin)
  {
    if((bin % 65536) == 0 && shouldCancel)
    {
      return {};
    }
    const float64 boundary = histogram.min + ((static_cast<float64>(bin) + 1.0) * histogram.binWidth);
    const float64 measurement = histogram.min + ((static_cast<float64>(bin) + 0.5) * histogram.binWidth);
    if(!std::isfinite(boundary) || !std::isfinite(measurement) || boundary <= previousBoundary || (bin != 0 && measurement <= previousMeasurement))
    {
      return MakeErrorResult(k_OtsuCollapsedBins, "Otsu histogram boundaries or measurements collapse at the selected precision. Rescale the finite input or use fewer histogram bins.");
    }
    previousBoundary = boundary;
    previousMeasurement = measurement;
  }
  return {};
}

template <class T>
struct OtsuFrequencyEntry
{
  T Value{};
  usize Count = 0;
};

template <class T>
struct OtsuFrequencyRing
{
  CacheMemoryBudgetManager::WorkingMemoryReservation Reservation;
  std::unique_ptr<std::vector<OtsuFrequencyEntry<T>>> Frequencies;
  bool Compact = false;
};

template <class T>
Result<> AccumulateCheckedOtsuValue(Histogram& histogram, T value, T minValue, T maxValue, usize frequency = 1)
{
  usize bin = 0;
  if(value >= maxValue && minValue != maxValue)
  {
    bin = histogram.counts.size() - 1;
  }
  else if(value > minValue && minValue != maxValue)
  {
    const float64 coordinate = (static_cast<float64>(value) - histogram.min) * (1.0 / histogram.binWidth);
    const float64 lastBin = static_cast<float64>(histogram.counts.size() - 1);
    if(!std::isfinite(coordinate) || coordinate < 0.0)
    {
      return MakeErrorResult(k_OtsuInvalidBinCoordinate, "Otsu produced an unusable histogram coordinate for a selected finite value. Rescale the input before thresholding.");
    }
    // Narrow only a finite value already inside the bin-index range. Endpoint rounding cannot escape the histogram.
    bin = coordinate >= lastBin ? histogram.counts.size() - 1 : static_cast<usize>(coordinate);
  }
  histogram.counts[bin] += frequency;
  return {};
}
} // namespace detail

/**
 * @brief Builds the selected Otsu histogram without changing generic histogram consumers.
 * @tparam T Numeric input value type.
 * @tparam MaskT Boolean or uint8 selection-mask type.
 * @param input Scalar source values.
 * @param maskPtr Optional scalar mask with the input tuple count, paired in flat order. Null selects every input value.
 * @param options Histogram-bin count and transfer-memory target.
 * @param shouldCancel Requests cancellation during scans and histogram construction.
 * @return Histogram and number of contributing samples, or an input, numeric, or transfer error.
 * A valid cancellation result can be incomplete. The caller must check shouldCancel before use.
 *
 * @pre An active mask has the same tuple count as the input.
 *
 * Mask selection precedes nonfinite and range checks. Compact native integer ranges can retain exact frequencies in a fixed ring.
 * The ring uses capacity left after the input and mask buffers. A scan without usable ring frequencies uses a second bounded pass for nonconstant input.
 */
template <class T, class MaskT = uint8>
Result<OtsuHistogramResult> ComputeOtsuHistogram(const AbstractDataStore<T>& input, const AbstractDataStore<MaskT>* maskPtr, const OtsuImageOptions& options, const std::atomic_bool& shouldCancel)
{
  if(options.NumberOfBins < 2)
  {
    return MakeErrorResult<OtsuHistogramResult>(k_OtsuTooFewBins, "Otsu requires at least two histogram bins. Increase Number Of Histogram Bins.");
  }
  if(static_cast<usize>(options.NumberOfBins) > std::vector<usize>{}.max_size())
  {
    return MakeErrorResult<OtsuHistogramResult>(k_OtsuBinAllocationTooLarge, "Otsu histogram bin storage exceeds the addressable allocation size. Reduce Number Of Histogram Bins.");
  }
  if(input.getNumberOfComponents() != 1 || (maskPtr != nullptr && maskPtr->getNumberOfComponents() != 1))
  {
    return MakeErrorResult<OtsuHistogramResult>(k_OtsuNonScalarStore, "Otsu input and optional mask must be scalar. Select arrays with one component per tuple.");
  }
  if(shouldCancel)
  {
    return {};
  }
  OtsuHistogramResult result;
  result.HistogramData.counts.assign(options.NumberOfBins, 0);
  ArrayMinMax<T> bounds;
  const bool external = input.getStoreType() == IDataStore::StoreType::OutOfCore || (maskPtr != nullptr && maskPtr->getStoreType() == IDataStore::StoreType::OutOfCore);
  const usize total = input.getSize();
  const usize bytesPerValue = sizeof(T) + (maskPtr == nullptr ? 0 : sizeof(MaskT));
  auto plan = detail::ReserveOtsuBuffers(
      options, {.TotalValues = total, .BytesPerValue = bytesPerValue, .UsesExternalStore = external, .SummaryBytesPerBlock = std::is_floating_point_v<T> ? sizeof(ArrayMinMax<T>) : 0});
  constexpr usize ringSize = 65536;
  using FrequencyRingType = std::conditional_t<std::is_integral_v<T>, detail::OtsuFrequencyRing<T>, const detail::OtsuFrequencyRing<T>>;
  FrequencyRingType ring{};
  if constexpr(std::is_integral_v<T>)
  {
    const usize ringBytes = ringSize * sizeof(detail::OtsuFrequencyEntry<T>);
    // Admit the optional ring in full only after the required payload, without evicting cache entries.
    if(external && ResolvePreferredWorkingMemoryBytes(ringBytes) >= ringBytes)
    {
      ring.Reservation = CacheMemoryBudgetManager::instance().tryReserveWorkingMemoryWithoutEviction(ringBytes);
    }
    ring.Compact = !external || ring.Reservation.sizeBytes() == ringBytes;
    if(ring.Compact)
    {
      ring.Frequencies = std::make_unique<std::vector<detail::OtsuFrequencyEntry<T>>>(ringSize);
    }
  }
  Result<> rangeResult = detail::VisitOtsuValues(
      input, maskPtr, options, shouldCancel, plan.Values,
      [&](nonstd::span<const T> values, nonstd::span<const MaskT> maskValues, usize) -> Result<> {
        if constexpr(std::is_floating_point_v<T>)
        {
          bounds.merge(detail::ReduceOtsuFloatingRange(values, maskValues, shouldCancel));
          return {};
        }
        else
        {
          for(usize i = 0; i < values.size(); ++i)
          {
            if((i % 65536) == 0 && shouldCancel)
            {
              return {};
            }
            if(maskPtr != nullptr && !maskValues[i])
            {
              continue;
            }
            const T value = values[i];
            bounds.addValue(value);
            if constexpr(std::is_integral_v<T>)
            {
              if(ring.Compact)
              {
                using KeyType = detail::UnsignedIntegralType<T>;
                const KeyType key = detail::IntegralMonotonicKey(value);
                ring.Compact = (detail::IntegralMonotonicKey(bounds.max) - detail::IntegralMonotonicKey(bounds.min)) <= static_cast<KeyType>(ringSize - 1);
                if(ring.Compact)
                {
                  auto& entry = (*ring.Frequencies)[static_cast<usize>(key & static_cast<KeyType>(ringSize - 1))];
                  if(entry.Count != 0 && entry.Value != value)
                  {
                    ring.Compact = false;
                  }
                  else
                  {
                    entry.Value = value;
                    ++entry.Count;
                  }
                }
                if(!ring.Compact)
                {
                  ring.Frequencies.reset();
                  ring.Reservation = {};
                }
              }
            }
          }
          return {};
        }
      },
      std::is_floating_point_v<T>);
  const T minValue = bounds.min;
  const T maxValue = bounds.max;
  result.FiniteCount = bounds.count;
  if(std::is_floating_point_v<T> || ring.Compact || minValue == maxValue)
  {
    plan.Reservation = {};
  }
  if(rangeResult.invalid())
  {
    return ConvertInvalidResult<OtsuHistogramResult>(std::move(rangeResult));
  }
  if(shouldCancel)
  {
    return ConvertResultTo(std::move(rangeResult), std::move(result));
  }
  if(result.FiniteCount == 0)
  {
    return ConvertResultTo(std::move(rangeResult), std::move(result));
  }
  auto& histogram = result.HistogramData;
  histogram.min = static_cast<float64>(minValue);
  histogram.max = static_cast<float64>(maxValue);
  Result<> geometryResult = detail::ConfigureCheckedOtsuBins(histogram, minValue == maxValue, shouldCancel);
  if(geometryResult.invalid())
  {
    return ConvertResultTo(MergeResults(std::move(rangeResult), std::move(geometryResult)), std::move(result));
  }
  if(shouldCancel)
  {
    return ConvertResultTo(std::move(rangeResult), std::move(result));
  }
  if(minValue == maxValue)
  {
    histogram.counts.front() = result.FiniteCount;
  }
  else if(ring.Compact)
  {
    for(usize i = 0; i < ringSize; ++i)
    {
      if(shouldCancel)
      {
        return ConvertResultTo(std::move(rangeResult), std::move(result));
      }
      const auto& entry = (*ring.Frequencies)[i];
      if(entry.Count != 0)
      {
        Result<> addResult = detail::AccumulateCheckedOtsuValue(histogram, entry.Value, minValue, maxValue, entry.Count);
        if(addResult.invalid())
        {
          return ConvertResultTo(MergeResults(std::move(rangeResult), std::move(addResult)), std::move(result));
        }
      }
    }
  }
  else
  {
    if constexpr(std::is_floating_point_v<T>)
    {
      // The floating-point histogram pass needs no range summaries.
      plan = detail::ReserveOtsuBuffers(options, {.TotalValues = total, .BytesPerValue = bytesPerValue, .UsesExternalStore = external});
    }
    Result<> histogramResult = detail::VisitOtsuValues(input, maskPtr, options, shouldCancel, plan.Values, [&](nonstd::span<const T> values, nonstd::span<const MaskT> maskValues, usize) -> Result<> {
      for(usize i = 0; i < values.size(); ++i)
      {
        if((i % 65536) == 0 && shouldCancel)
        {
          return {};
        }
        if((maskPtr != nullptr && !maskValues[i]) || !std::isfinite(static_cast<float64>(values[i])))
        {
          continue;
        }
        Result<> addResult = detail::AccumulateCheckedOtsuValue(histogram, values[i], minValue, maxValue);
        if(addResult.invalid())
        {
          return addResult;
        }
      }
      return {};
    });
    rangeResult = MergeResults(std::move(rangeResult), std::move(histogramResult));
    if(rangeResult.invalid())
    {
      return ConvertInvalidResult<OtsuHistogramResult>(std::move(rangeResult));
    }
  }
  return ConvertResultTo(std::move(rangeResult), std::move(result));
}

/**
 * @brief Runs the existing finite Otsu solver with opt-in numeric checks and cooperative cancellation.
 * @param histogram Selected histogram with usable bounds, bin width, and counts.
 * @param options Threshold count, Valley Emphasis, and bin-midpoint selection.
 * @param shouldCancel Requests cancellation during threshold search.
 * @return Ordered threshold values, or a configuration or numeric error. Cancellation can return a valid empty vector.
 * The caller must check shouldCancel before classification.
 *
 * A true constant uses repeated thresholds at that constant. Finite tie and Valley Emphasis rules remain unchanged.
 */
inline Result<std::vector<float64>> ComputeCheckedOtsuThresholds(const Histogram& histogram, const OtsuImageOptions& options, const std::atomic_bool& shouldCancel)
{
  if(shouldCancel)
  {
    return {};
  }
  if(options.NumberOfThresholds == 0 || options.NumberOfThresholds >= histogram.counts.size())
  {
    return MakeErrorResult<std::vector<float64>>(k_OtsuInvalidThresholdCount, "Otsu Number Of Thresholds must be positive and less than the histogram bin count.");
  }
  if(std::ranges::all_of(histogram.counts, [](usize count) { return count == 0; }))
  {
    return MakeErrorResult<std::vector<float64>>(k_OtsuEmptyHistogram, "Otsu threshold calculation requires at least one histogram sample.");
  }
  if(!std::isfinite(histogram.min) || !std::isfinite(histogram.max) || !std::isfinite(histogram.binWidth) || histogram.binWidth < 0.0)
  {
    return MakeErrorResult<std::vector<float64>>(k_OtsuInvalidHistogram,
                                                 "Otsu threshold calculation requires finite histogram bounds and a nonnegative finite bin width. Rebuild the histogram from selected finite data.");
  }
  if(histogram.binWidth == 0.0 && histogram.min == histogram.max)
  {
    return {{std::vector<float64>(options.NumberOfThresholds, histogram.min)}};
  }
  return detail::ComputeOtsuThresholds<true>(histogram, options.NumberOfThresholds, options.ValleyEmphasis, options.ReturnBinMidpoint, &shouldCancel);
}

/**
 * @brief Classifies a selected value. A finite value equal to a threshold stays in the lower class.
 * @tparam T Numeric input value type.
 * @param value Native value to classify.
 * @param thresholds Finite thresholds in ascending order, or an empty vector when no samples contributed.
 * @param options Class offset, threshold count, and excluded label. The offset plus the threshold count must not exceed 255.
 * @return A class label. The caller applies explicit mask exclusions.
 * @pre The caller has validated the class interval. An empty threshold vector selects the lowest class.
 */
template <class T>
uint8 ClassifyOtsuValue(T value, const std::vector<float64>& thresholds, const OtsuImageOptions& options)
{
  // Strict comparison puts a tie in the lower class. The sorted thresholds are partitioned for each input, including NaN.
  const auto firstUncrossed = std::ranges::partition_point(thresholds, [value](float64 threshold) { return threshold < static_cast<float64>(value); });
  const usize index = static_cast<usize>(firstUncrossed - thresholds.begin());
  return static_cast<uint8>(static_cast<usize>(options.LabelOffset) + index);
}

/**
 * @brief Fits an Otsu histogram and writes selected classes and an excluded label.
 * @tparam T Numeric input value type.
 * @tparam MaskT Boolean or uint8 selection-mask type.
 * @param input Scalar source image.
 * @param output Scalar uint8 destination with the input value count.
 * @param maskPtr Optional scalar mask with the input tuple count, paired in flat order. Null selects every value.
 * @param options Histogram, class-label and transfer controls.
 * @param shouldCancel Requests cancellation during scans, search, and output work.
 * @param messageHandler Receives output-phase progress messages.
 * @return Collected transfer diagnostics or a validation or numeric error.
 * Cancellation retains the valid engine-result convention.
 *
 * @pre The class labels fit in uint8, the threshold count is positive and less than the bin count, Excluded Label is distinct when used, and the mask has the input tuple count.
 *
 * Only selected finite values contribute to the histogram. The optional mask is read in bounded owned chunks.
 * A later write failure or cancellation can leave completed output batches in place.
 */
template <class T, class MaskT = uint8>
Result<> ApplyOtsuImage(const AbstractDataStore<T>& input, AbstractDataStore<uint8>& output, const AbstractDataStore<MaskT>* maskPtr, const OtsuImageOptions& options,
                        const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  if(input.getSize() != output.getSize() || output.getNumberOfComponents() != 1)
  {
    return MakeErrorResult(k_OtsuInvalidOutputShape, "Otsu output must be scalar and match the input tuple count. Recreate the output array before thresholding.");
  }
  auto histogramResult = ComputeOtsuHistogram(input, maskPtr, options, shouldCancel);
  if(histogramResult.invalid())
  {
    return ConvertResult(std::move(histogramResult));
  }
  if(shouldCancel || input.getSize() == 0)
  {
    return ConvertResult(std::move(histogramResult));
  }
  Result<> result;
  result.warnings() = std::move(histogramResult.warnings());
  Result<std::vector<float64>> thresholdsResult;
  if(histogramResult.value().FiniteCount != 0)
  {
    thresholdsResult = ComputeCheckedOtsuThresholds(histogramResult.value().HistogramData, options, shouldCancel);
  }
  if(thresholdsResult.invalid())
  {
    return MergeResults(std::move(result), ConvertResult(std::move(thresholdsResult)));
  }
  if(shouldCancel)
  {
    return result;
  }
  const auto& thresholds = thresholdsResult.value();
  if(maskPtr == nullptr)
  {
    result = MergeResults(std::move(result), ApplyPointwise<T, uint8>(input, output, [&](T value) { return ClassifyOtsuValue(value, thresholds, options); }, shouldCancel, messageHandler));
  }
  else
  {
    const bool external =
        input.getStoreType() == IDataStore::StoreType::OutOfCore || maskPtr->getStoreType() == IDataStore::StoreType::OutOfCore || output.getStoreType() == IDataStore::StoreType::OutOfCore;
    auto plan = detail::ReserveOtsuBuffers(options, {.TotalValues = input.getSize(), .BytesPerValue = sizeof(T) + sizeof(MaskT) + sizeof(uint8), .UsesExternalStore = external});
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays) -- A full transfer overwrites this contiguous buffer, including for bool.
    auto valuesPtr = std::make_unique_for_overwrite<T[]>(plan.Values);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays) -- A full mask transfer overwrites the contiguous bool or uint8 buffer.
    auto maskValuesPtr = std::make_unique_for_overwrite<MaskT[]>(plan.Values);
    auto labelsPtr = std::make_unique<std::vector<uint8>>(plan.Values);
    messageHandler.sendInfoMessage("Applying masked Otsu labels in bounded batches");
    ThrottledMessageHandler progress(messageHandler);
    progress.reset(1 + ((input.getSize() - 1) / plan.Values), "Applying masked Otsu labels");
    for(usize start = 0; start < input.getSize();)
    {
      if(shouldCancel)
      {
        return result;
      }
      const usize count = std::min(plan.Values, input.getSize() - start);
      Result<> maskReadResult = maskPtr->copyIntoBuffer(start, nonstd::span<MaskT>(maskValuesPtr.get(), count));
      result = MergeResults(std::move(result), std::move(maskReadResult));
      if(result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return result;
      }
      Result<> inputReadResult = input.copyIntoBuffer(start, nonstd::span<T>(valuesPtr.get(), count));
      result = MergeResults(std::move(result), std::move(inputReadResult));
      if(result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return result;
      }
      ParallelDataAlgorithm algorithm;
      algorithm.setRange(0, count);
      if(count <= detail::k_OtsuMaxSerialLabelPixels)
      {
        algorithm.setParallelizationEnabled(false);
      }
      algorithm.execute([&](const Range& range) {
        for(usize i = range.min(); i < range.max(); ++i)
        {
          if((i % 65536) == 0 && shouldCancel)
          {
            return;
          }
          (*labelsPtr)[i] = maskValuesPtr[i] ? ClassifyOtsuValue(valuesPtr[i], thresholds, options) : options.ExcludedLabel;
        }
      });
      if(shouldCancel)
      {
        return result;
      }
      Result<> writeResult = output.copyFromBuffer(start, nonstd::span<const uint8>(labelsPtr->data(), count));
      result = MergeResults(std::move(result), std::move(writeResult));
      if(result.invalid())
      {
        return result;
      }
      start += count;
      progress.incrementPercent(1, 1);
    }
  }
  return result;
}
} // namespace nx::core::ImageProcessing
