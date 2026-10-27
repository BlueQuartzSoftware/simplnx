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
/**
 * @struct OtsuImageOptions
 * @brief Holds Otsu controls for fixed uint8 output labels.
 */
struct OtsuImageOptions
{
  uint32 NumberOfBins = 128;
  uint8 NumberOfThresholds = 1;
  uint8 LabelOffset = 0;
  uint8 MaskedLabel = 255;
  bool WarnOnNonfinite = false;
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
  usize NaNCount = 0;
  usize NegativeInfinityCount = 0;
  usize PositiveInfinityCount = 0;
};

namespace detail
{
// Match the existing floating statistics reduction grain; batch caps also bound the summary table.
inline constexpr usize k_OtsuRangeBlockValues = 262144;
inline constexpr usize k_OtsuMaximumBatchBytes = 64ULL * 1024ULL * 1024ULL;

template <class T>
struct OtsuFloatingRangeSummary
{
  T Minimum{};
  T Maximum{};
  usize FiniteCount = 0;
  usize NaNCount = 0;
  usize NegativeInfinityCount = 0;
  usize PositiveInfinityCount = 0;
};

template <class T, class MaskT>
OtsuFloatingRangeSummary<T> ReduceOtsuFloatingBlock(nonstd::span<const T> values, nonstd::span<const MaskT> maskValues, const std::atomic_bool& shouldCancel)
{
  OtsuFloatingRangeSummary<T> summary;
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
      continue;
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

template <class T>
void MergeOtsuFloatingSummary(OtsuFloatingRangeSummary<T>& summary, const OtsuFloatingRangeSummary<T>& block)
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

template <class T, class MaskT>
OtsuFloatingRangeSummary<T> ReduceOtsuFloatingRange(nonstd::span<const T> values, nonstd::span<const MaskT> maskValues, const std::atomic_bool& shouldCancel)
{
  if(values.size() <= k_OtsuRangeBlockValues)
  {
    return ReduceOtsuFloatingBlock(values, maskValues, shouldCancel);
  }
  // The caller caps each resident view/owned batch. This table never has one entry per full-image block.
  const usize blockCount = 1 + ((values.size() - 1) / k_OtsuRangeBlockValues);
  std::vector<OtsuFloatingRangeSummary<T>> summaries(blockCount);
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
  OtsuFloatingRangeSummary<T> summary;
  for(const auto& block : summaries)
  {
    if(shouldCancel)
    {
      return {};
    }
    MergeOtsuFloatingSummary(summary, block);
  }
  return summary;
}

struct OtsuBufferPlan
{
  CacheMemoryBudgetManager::WorkingMemoryReservation Reservation;
  usize Values = 1;
};

inline OtsuBufferPlan ReserveOtsuBuffers(usize total, usize bytesPerValue, usize targetBytes, bool usesExternalStore, usize summaryBytesPerBlock = 0)
{
  OtsuBufferPlan plan;
  const usize usefulBytes = total > std::numeric_limits<usize>::max() / bytesPerValue ? std::numeric_limits<usize>::max() : total * bytesPerValue;
  usize bytes = std::min({targetBytes, k_OtsuMaximumBatchBytes, usefulBytes});
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
Result<> VisitOtsuValues(const AbstractDataStore<T>& input, const AbstractDataStore<MaskT>* maskPtr, const OtsuImageOptions& options, const std::atomic_bool& shouldCancel, const VisitorT& visitor,
                         bool parallelRange = false)
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
      status = MergeResults(std::move(status), visitor(values.subspan(start, count), maskPtr == nullptr ? nonstd::span<const MaskT>{} : maskValues.subspan(start, count)));
      if(status.invalid())
      {
        return status;
      }
      start += count;
    }
    return status;
  }

  auto plan = ReserveOtsuBuffers(total, sizeof(T) + (maskPtr == nullptr ? 0 : sizeof(MaskT)), options.TargetBytes,
                                 input.getStoreType() == IDataStore::StoreType::OutOfCore || (maskPtr != nullptr && maskPtr->getStoreType() == IDataStore::StoreType::OutOfCore),
                                 parallelRange ? sizeof(OtsuFloatingRangeSummary<T>) : 0);
  auto valuesPtr = std::make_unique_for_overwrite<T[]>(plan.Values);
  auto maskValuesPtr = maskPtr == nullptr ? nullptr : std::make_unique_for_overwrite<MaskT[]>(plan.Values);
  for(usize start = 0; start < total;)
  {
    if(shouldCancel)
    {
      return status;
    }
    const usize count = std::min(plan.Values, total - start);
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
                          visitor(nonstd::span<const T>(valuesPtr.get(), count), maskPtr == nullptr ? nonstd::span<const MaskT>{} : nonstd::span<const MaskT>(maskValuesPtr.get(), count)));
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
    return MakeErrorResult(-8326, "Otsu cannot form a finite positive histogram range/bin width from the selected native values. Rescale the finite input or use fewer histogram bins.");
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
      return MakeErrorResult(-8327, "Otsu histogram boundaries or measurements collapse at the selected precision. Rescale the finite input or use fewer histogram bins.");
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
      return MakeErrorResult(-8328, "Otsu produced an unusable histogram coordinate for a selected finite value. Rescale the input before thresholding.");
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
 * @param options Histogram-bin count, nonfinite policy, and transfer-memory target.
 * @param shouldCancel Requests cancellation during scans and histogram construction.
 * @return Histogram and selected finite and nonfinite sample counts, or an input, numeric, or transfer error.
 * A valid cancellation result can be incomplete. The caller must check shouldCancel before use.
 *
 * Mask selection precedes nonfinite and range checks. Compact native integer ranges retain exact frequencies in a fixed ring.
 * Other ranges use a second bounded pass.
 */
template <class T, class MaskT = uint8>
Result<OtsuHistogramResult> ComputeOtsuHistogram(const AbstractDataStore<T>& input, const AbstractDataStore<MaskT>* maskPtr, const OtsuImageOptions& options, const std::atomic_bool& shouldCancel)
{
  if(options.NumberOfBins < 2)
  {
    return MakeErrorResult<OtsuHistogramResult>(-28600, "Otsu requires at least two histogram bins. Increase Number Of Histogram Bins.");
  }
  if(static_cast<usize>(options.NumberOfBins) > std::vector<usize>{}.max_size())
  {
    return MakeErrorResult<OtsuHistogramResult>(-28604, "Otsu histogram bin storage exceeds the addressable allocation size. Reduce Number Of Histogram Bins.");
  }
  if(input.getNumberOfComponents() != 1 || (maskPtr != nullptr && (maskPtr->getNumberOfComponents() != 1 || maskPtr->getSize() != input.getSize())))
  {
    return MakeErrorResult<OtsuHistogramResult>(-8329, "Otsu input and optional mask must be scalar and contain the same number of tuples. Select a scalar mask with one value per input tuple.");
  }
  OtsuHistogramResult result;
  if(shouldCancel)
  {
    return {std::move(result)};
  }
  result.HistogramData.counts.assign(options.NumberOfBins, 0);
  T minValue{};
  T maxValue{};
  constexpr usize ringSize = 65536;
  std::unique_ptr<detail::OtsuFrequencyEntry<T>[]> frequenciesPtr;
  CacheMemoryBudgetManager::WorkingMemoryReservation ringReservation;
  using CompactRangeType = std::conditional_t<std::is_integral_v<T>, bool, const bool>;
  CompactRangeType compactRange = false;
  if constexpr(std::is_integral_v<T>)
  {
    const bool external = input.getStoreType() == IDataStore::StoreType::OutOfCore || (maskPtr != nullptr && maskPtr->getStoreType() == IDataStore::StoreType::OutOfCore);
    const usize ringBytes = ringSize * sizeof(detail::OtsuFrequencyEntry<T>);
    if(external)
    {
      ringReservation = ReserveWorkingMemory(ringBytes, ringBytes);
    }
    compactRange = !external || ringReservation.sizeBytes() >= ringBytes;
    if(compactRange)
    {
      frequenciesPtr = std::make_unique<detail::OtsuFrequencyEntry<T>[]>(ringSize);
    }
    else
    {
      ringReservation = {};
    }
  }
  Result<> rangeResult = detail::VisitOtsuValues(
      input, maskPtr, options, shouldCancel,
      [&](nonstd::span<const T> values, nonstd::span<const MaskT> maskValues) -> Result<> {
        if constexpr(std::is_floating_point_v<T>)
        {
          const auto summary = detail::ReduceOtsuFloatingRange(values, maskValues, shouldCancel);
          if(shouldCancel)
          {
            return {};
          }
          if(!options.WarnOnNonfinite && (summary.NaNCount != 0 || summary.NegativeInfinityCount != 0 || summary.PositiveInfinityCount != 0))
          {
            return MakeErrorResult(-8324, "Otsu found a selected NaN or infinity. Choose Warn and Continue to retain endpoint-bin participation, or explicitly exclude it with Use Mask Array.");
          }
          if(summary.FiniteCount != 0)
          {
            if(result.FiniteCount == 0)
            {
              minValue = summary.Minimum;
              maxValue = summary.Maximum;
            }
            else
            {
              minValue = std::min(minValue, summary.Minimum);
              maxValue = std::max(maxValue, summary.Maximum);
            }
          }
          result.FiniteCount += summary.FiniteCount;
          result.NaNCount += summary.NaNCount;
          result.NegativeInfinityCount += summary.NegativeInfinityCount;
          result.PositiveInfinityCount += summary.PositiveInfinityCount;
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
            if(result.FiniteCount == 0)
            {
              minValue = value;
              maxValue = value;
            }
            else
            {
              minValue = std::min(minValue, value);
              maxValue = std::max(maxValue, value);
            }
            ++result.FiniteCount;
            if constexpr(std::is_integral_v<T>)
            {
              if(compactRange)
              {
                using KeyType = detail::UnsignedIntegralType<T>;
                const KeyType key = detail::IntegralMonotonicKey(value);
                compactRange = (detail::IntegralMonotonicKey(maxValue) - detail::IntegralMonotonicKey(minValue)) <= static_cast<KeyType>(ringSize - 1);
                if(compactRange)
                {
                  auto& entry = frequenciesPtr[static_cast<usize>(key & static_cast<KeyType>(ringSize - 1))];
                  if(entry.Count != 0 && entry.Value != value)
                  {
                    compactRange = false;
                  }
                  else
                  {
                    entry.Value = value;
                    ++entry.Count;
                  }
                }
              }
            }
          }
          return {};
        }
      },
      std::is_floating_point_v<T>);
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
    return ConvertResultTo(
        MergeResults(std::move(rangeResult), MakeErrorResult(-8325, "Otsu has no selected finite samples. Select at least one finite input value with the optional mask before thresholding.")),
        std::move(result));
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
  else if(compactRange)
  {
    for(usize i = 0; i < ringSize; ++i)
    {
      if(shouldCancel)
      {
        return ConvertResultTo(std::move(rangeResult), std::move(result));
      }
      const auto& entry = frequenciesPtr[i];
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
    frequenciesPtr.reset();
    ringReservation = {};
    Result<> histogramResult = detail::VisitOtsuValues(input, maskPtr, options, shouldCancel, [&](nonstd::span<const T> values, nonstd::span<const MaskT> maskValues) -> Result<> {
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
  histogram.counts.front() += result.NaNCount + result.NegativeInfinityCount;
  histogram.counts.back() += result.PositiveInfinityCount;
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
    return {std::vector<float64>{}};
  }
  if(options.NumberOfThresholds == 0 || options.NumberOfThresholds >= histogram.counts.size())
  {
    return MakeErrorResult<std::vector<float64>>(-28601, "Otsu Number Of Thresholds must be positive and less than the histogram bin count.");
  }
  if(std::ranges::all_of(histogram.counts, [](usize count) { return count == 0; }))
  {
    return MakeErrorResult<std::vector<float64>>(-28605, "Otsu threshold calculation requires at least one histogram sample.");
  }
  if(!std::isfinite(histogram.min) || !std::isfinite(histogram.max) || !std::isfinite(histogram.binWidth) || histogram.binWidth < 0.0)
  {
    return MakeErrorResult<std::vector<float64>>(-28606,
                                                 "Otsu threshold calculation requires finite histogram bounds and a nonnegative finite bin width. Rebuild the histogram from selected finite data.");
  }
  if(histogram.binWidth == 0.0 && histogram.min == histogram.max)
  {
    return {std::vector<float64>(options.NumberOfThresholds, histogram.min)};
  }
  return detail::ComputeOtsuThresholds<true>(histogram, options.NumberOfThresholds, options.ValleyEmphasis, options.ReturnBinMidpoint, &shouldCancel);
}

/**
 * @brief Maps one selected value to an ordinary uint8 class; equality stays in the lower class.
 * @tparam T Numeric input value type.
 * @param value Native value to classify. NaN and negative infinity use the lowest class; positive infinity uses the highest class.
 * @param thresholds Finite thresholds in ascending order, with exactly NumberOfThresholds entries.
 * @param options Class offset and threshold count. Their sum must not exceed 255.
 * @return The ordinary class label. Masked-label selection belongs to the caller.
 * @pre The caller has validated the class interval and threshold vector and has enforced the selected nonfinite policy.
 */
template <class T>
uint8 ClassifyOtsuValue(T value, const std::vector<float64>& thresholds, const OtsuImageOptions& options)
{
  if constexpr(std::is_floating_point_v<T>)
  {
    if(!std::isfinite(value))
    {
      return std::isinf(value) && value > 0 ? static_cast<uint8>(static_cast<uint32>(options.LabelOffset) + options.NumberOfThresholds) : options.LabelOffset;
    }
  }
  const usize index = static_cast<usize>(std::ranges::lower_bound(thresholds, static_cast<float64>(value)) - thresholds.begin());
  return static_cast<uint8>(static_cast<usize>(options.LabelOffset) + index);
}

/**
 * @brief Fits an Otsu histogram and writes selected classes and optional masked labels.
 * @tparam T Numeric input value type.
 * @tparam MaskT Boolean or uint8 selection-mask type.
 * @param input Scalar source image.
 * @param output Scalar uint8 destination with the input value count.
 * @param maskPtr Optional scalar mask with the input tuple count, paired in flat order. Null selects every value.
 * @param options Histogram, class-label, nonfinite, and transfer controls.
 * @param shouldCancel Requests cancellation during scans, search, and output work.
 * @param messageHandler Receives output-phase progress messages.
 * @return Collected diagnostics, including the nonfinite participation warning, or a validation, numeric, or transfer error.
 * Cancellation retains the valid engine-result convention.
 *
 * Mask selection precedes histogram and nonfinite checks. The optional mask is read in bounded owned chunks.
 * A later write failure or cancellation can leave completed output batches in place.
 */
template <class T, class MaskT = uint8>
Result<> ApplyOtsuImage(const AbstractDataStore<T>& input, AbstractDataStore<uint8>& output, const AbstractDataStore<MaskT>* maskPtr, const OtsuImageOptions& options,
                        const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  if(input.getSize() != output.getSize() || output.getNumberOfComponents() != 1)
  {
    return MakeErrorResult(-28602, "Otsu output must be scalar and match the input tuple count. Recreate the output array before thresholding.");
  }
  if(static_cast<uint32>(options.LabelOffset) + options.NumberOfThresholds > 255 ||
     (maskPtr != nullptr && options.MaskedLabel >= options.LabelOffset && static_cast<uint32>(options.MaskedLabel) <= static_cast<uint32>(options.LabelOffset) + options.NumberOfThresholds))
  {
    return MakeErrorResult(-28603, "Otsu labels do not fit a distinct uint8 class interval. Reduce Label Offset/Number Of Thresholds or select a Masked Label outside that interval.");
  }
  auto histogramResult = ComputeOtsuHistogram(input, maskPtr, options, shouldCancel);
  if(histogramResult.invalid())
  {
    return ConvertResult(std::move(histogramResult));
  }
  if(shouldCancel)
  {
    return ConvertResult(std::move(histogramResult));
  }
  Result<> result;
  result.warnings() = std::move(histogramResult.warnings());
  const auto& counts = histogramResult.value();
  if(counts.NaNCount != 0 || counts.NegativeInfinityCount != 0 || counts.PositiveInfinityCount != 0)
  {
    result.warnings().push_back(Warning{.code = -8319,
                                        .message = fmt::format("Otsu retained {} NaN, {} negative infinity and {} positive infinity samples in the endpoint histogram bins. Classification can be "
                                                               "unreliable or unpredictable. Use an explicit mask to exclude these samples.",
                                                               counts.NaNCount, counts.NegativeInfinityCount, counts.PositiveInfinityCount)});
  }
  auto thresholdsResult = ComputeCheckedOtsuThresholds(histogramResult.value().HistogramData, options, shouldCancel);
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
    auto plan = detail::ReserveOtsuBuffers(input.getSize(), sizeof(T) + sizeof(MaskT) + sizeof(uint8), options.TargetBytes,
                                           input.getStoreType() == IDataStore::StoreType::OutOfCore || maskPtr->getStoreType() == IDataStore::StoreType::OutOfCore ||
                                               output.getStoreType() == IDataStore::StoreType::OutOfCore);
    auto valuesPtr = std::make_unique_for_overwrite<T[]>(plan.Values);
    auto maskValuesPtr = std::make_unique_for_overwrite<MaskT[]>(plan.Values);
    auto labelsPtr = std::make_unique_for_overwrite<uint8[]>(plan.Values);
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
      algorithm.execute([&](const Range& range) {
        for(usize i = range.min(); i < range.max(); ++i)
        {
          if((i % 65536) == 0 && shouldCancel)
          {
            return;
          }
          labelsPtr[i] = maskValuesPtr[i] ? ClassifyOtsuValue(valuesPtr[i], thresholds, options) : options.MaskedLabel;
        }
      });
      if(shouldCancel)
      {
        return result;
      }
      Result<> writeResult = output.copyFromBuffer(start, nonstd::span<const uint8>(labelsPtr.get(), count));
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
