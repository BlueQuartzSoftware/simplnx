#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>

namespace nx::core::ImageProcessing
{
struct Histogram
{
  std::vector<usize> counts;
  float64 min = 0.0;
  float64 max = 0.0;
  float64 binWidth = 0.0;
};

struct HistogramScanOptions
{
  static constexpr usize k_DefaultTargetBytes = 64ULL * 1024ULL * 1024ULL;
  usize targetBytes = k_DefaultTargetBytes;
};

namespace detail
{
template <class T>
struct HistogramScanBuffer
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize values = 0;
};

template <class T>
HistogramScanBuffer<T> ReserveHistogramScanBuffer(const AbstractDataStore<T>& store, usize totalValues, const HistogramScanOptions& options)
{
  usize usefulBytes = 0;
  if(totalValues != 0 && totalValues > std::numeric_limits<usize>::max() / sizeof(T))
  {
    usefulBytes = std::numeric_limits<usize>::max();
  }
  else
  {
    usefulBytes = totalValues * sizeof(T);
  }
  if(store.getStoreType() != IDataStore::StoreType::OutOfCore)
  {
    const usize bytes = options.targetBytes == HistogramScanOptions::k_DefaultTargetBytes ? HistogramScanOptions::k_DefaultTargetBytes : options.targetBytes;
    return {{}, std::max<usize>(1, std::min(totalValues, bytes / sizeof(T)))};
  }
  const uint64 preferredBytes =
      options.targetBytes == HistogramScanOptions::k_DefaultTargetBytes ? ResolveWorkingMemoryFractionBytes(usefulBytes, 1, 4, HistogramScanOptions::k_DefaultTargetBytes) : options.targetBytes;
  auto reservation = ReserveWorkingMemory(preferredBytes, usefulBytes);
  const usize grantedBytes = static_cast<usize>(std::min<uint64>(reservation.sizeBytes(), std::numeric_limits<usize>::max()));
  return {std::move(reservation), std::max<usize>(1, std::min(totalValues, grantedBytes / sizeof(T)))};
}

inline Histogram InitializeHistogram(usize numBins, float64 min, float64 max)
{
  Histogram histogram;
  histogram.min = min;
  histogram.max = max;
  histogram.counts.assign(numBins, 0);
  return histogram;
}

/**
 * @brief Configures the uniform bin width exactly like ITK's Histogram<double> path.
 */
inline void ConfigureHistogramBinning(Histogram& histogram)
{
  histogram.binWidth = 0.0;
  const float64 range = histogram.max - histogram.min;
  if(range > 0.0)
  {
    // ITK's histogram is always Histogram<double>, so itk::SampleToHistogramFilter always takes its non-integer
    // branch and sets the upper bound to max + (max-min)/(numBins*marginalScale) with marginalScale == 100 (its
    // default), slightly widening every bin so a value exactly == max falls inside the last bin without a clamp.
    // Reproduce that widening so bin assignments AND the returned threshold values match ITK bit-for-bit on
    // continuous-intensity data (the lower bound stays == min).
    constexpr float64 k_MarginalScale = 100.0;
    const float64 upper = histogram.max + range / (static_cast<float64>(histogram.counts.size()) * k_MarginalScale);
    histogram.binWidth = (upper - histogram.min) / static_cast<float64>(histogram.counts.size());
  }
}

/**
 * @brief Adds @p frequency occurrences of one value using the shared ITK-compatible bin formula.
 */
inline void AccumulateHistogramValue(Histogram& histogram, float64 value, float64 invBinWidth, int64 maxBin, usize frequency = 1)
{
  if(!std::isfinite(value))
  {
    return;
  }

  int64 bin = static_cast<int64>((value - histogram.min) * invBinWidth);
  if(bin < 0)
  {
    bin = 0;
  }
  if(bin > maxBin)
  {
    bin = maxBin;
  }
  histogram.counts[static_cast<usize>(bin)] += frequency;
}

template <class T>
using UnsignedIntegralType = std::make_unsigned_t<T>;

template <class T>
constexpr UnsignedIntegralType<T> IntegralMonotonicKey(T value) noexcept
{
  using Key = UnsignedIntegralType<T>;
  Key key = static_cast<Key>(value);
  if constexpr(std::is_signed_v<T>)
  {
    constexpr Key k_SignBit = Key{1} << (std::numeric_limits<Key>::digits - 1);
    key ^= k_SignBit;
  }
  return key;
}

template <class T>
float64 IntegralKeyAsFloat64(UnsignedIntegralType<T> key) noexcept
{
  using Key = UnsignedIntegralType<T>;
  if constexpr(std::is_signed_v<T>)
  {
    constexpr Key k_SignBit = Key{1} << (std::numeric_limits<Key>::digits - 1);
    if(key < k_SignBit)
    {
      return -static_cast<float64>(k_SignBit - key);
    }
    return static_cast<float64>(key - k_SignBit);
  }
  return static_cast<float64>(key);
}
} // namespace detail

/**
 * @brief Bins every value of @p store into @p numBins uniform bins over [min, max].
 *
 * Values are clamped into range, so @p max lands in the last bin. The byte-capped buffer keeps memory
 * bounded.
 * The serial bulk I/O, cancel checks, and transfer-error propagation mirror StreamingStatistics.hpp.
 */
template <class T>
Result<Histogram> ComputeHistogram(const AbstractDataStore<T>& store, usize numBins, float64 min, float64 max, const std::atomic_bool& shouldCancel, const HistogramScanOptions& options = {})
{
  Histogram h = detail::InitializeHistogram(numBins, min, max);
  const usize total = store.getSize();
  if(numBins == 0 || total == 0)
  {
    return {h};
  }
  detail::ConfigureHistogramBinning(h);
  if(h.binWidth <= 0.0)
  {
    h.counts[0] = total; // degenerate range (min==max): all mass goes to bin 0
    return {h};
  }
  const float64 invBinWidth = 1.0 / h.binWidth; // hoist: multiply per value, don't divide
  const int64 maxBin = static_cast<int64>(numBins) - 1;

  auto scanBuffer = detail::ReserveHistogramScanBuffer<T>(store, total, options);
  auto buffer = std::make_unique_for_overwrite<T[]>(scanBuffer.values);
  scanBuffer.reservation.shrinkTo(scanBuffer.values * sizeof(T));
  for(usize start = 0; start < total; start += scanBuffer.values)
  {
    if(shouldCancel)
    {
      return {h};
    }
    const usize count = std::min(scanBuffer.values, total - start);
    if(Result<> r = store.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
    {
      return {nonstd::make_unexpected(std::move(r.errors()))};
    }
    constexpr usize k_CancelCheckValues = 65536;
    for(usize blockStart = 0; blockStart < count; blockStart += k_CancelCheckValues)
    {
      if(shouldCancel)
      {
        return {h};
      }
      const usize blockEnd = blockStart + std::min(k_CancelCheckValues, count - blockStart);
      for(usize i = blockStart; i < blockEnd; ++i)
      {
        detail::AccumulateHistogramValue(h, static_cast<float64>(buffer[i]), invBinWidth, maxBin);
      }
    }
  }
  return {h};
}

/**
 * @brief Computes an integral histogram while discovering the exact minimum and maximum.
 *
 * A fixed exact-frequency ring makes spans of at most 65,535 a single bounded bulk-I/O pass. Wider
 * ranges
 * finish the min/max pass and fall back to @ref ComputeHistogram for the same exact two-pass result.
 * Signed values use a monotonic unsigned key formed by flipping the sign bit. The
 * implementation avoids signed
 * subtraction and implementation-defined unsigned-to-signed reconstruction.
 */
template <class T>
Result<Histogram> ComputeIntegralHistogram(const AbstractDataStore<T>& store, usize numBins, const std::atomic_bool& shouldCancel, const HistogramScanOptions& options = {})
{
  static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>, "ComputeIntegralHistogram requires a non-boolean integral type");
  using Key = detail::UnsignedIntegralType<T>;

  constexpr usize k_RingSize = 65536;
  struct FrequencyEntry
  {
    Key key{};
    usize count = 0;
  };

  Histogram histogram = detail::InitializeHistogram(numBins, 0.0, 0.0);
  const usize total = store.getSize();
  if(total == 0 || numBins == 0 || shouldCancel)
  {
    return {histogram};
  }

  auto frequencies = std::make_unique<FrequencyEntry[]>(k_RingSize);
  bool first = true;
  bool compactRange = true;
  T minValue{};
  T maxValue{};
  Key minKey{};
  Key maxKey{};

  auto scanBuffer = detail::ReserveHistogramScanBuffer<T>(store, total, options);
  {
    auto buffer = std::make_unique_for_overwrite<T[]>(scanBuffer.values);
    scanBuffer.reservation.shrinkTo(scanBuffer.values * sizeof(T));
    for(usize start = 0; start < total; start += scanBuffer.values)
    {
      if(shouldCancel)
      {
        return {histogram};
      }
      const usize count = std::min(scanBuffer.values, total - start);
      if(Result<> readResult = store.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); readResult.invalid())
      {
        return {nonstd::make_unexpected(std::move(readResult.errors()))};
      }

      constexpr usize k_CancelCheckValues = 65536;
      for(usize blockStart = 0; blockStart < count; blockStart += k_CancelCheckValues)
      {
        if(shouldCancel)
        {
          return {histogram};
        }
        const usize blockEnd = blockStart + std::min(k_CancelCheckValues, count - blockStart);
        for(usize i = blockStart; i < blockEnd; ++i)
        {
          const T value = buffer[i];
          const Key key = detail::IntegralMonotonicKey(value);
          if(first)
          {
            minValue = value;
            maxValue = value;
            minKey = key;
            maxKey = key;
            first = false;
          }
          else
          {
            if(key < minKey)
            {
              minValue = value;
              minKey = key;
            }
            if(key > maxKey)
            {
              maxValue = value;
              maxKey = key;
            }
          }

          if(compactRange)
          {
            constexpr Key k_MaxCompactSpan = static_cast<Key>(k_RingSize - 1);
            compactRange = (maxKey - minKey) <= k_MaxCompactSpan;
            if(compactRange)
            {
              constexpr Key k_RingMask = static_cast<Key>(k_RingSize - 1);
              FrequencyEntry& entry = frequencies[static_cast<usize>(key & k_RingMask)];
              if(entry.count == 0)
              {
                entry.key = key;
              }
              else if(entry.key != key)
              {
                compactRange = false;
                continue;
              }
              ++entry.count;
            }
          }
        }
      }
    }
  }

  if(shouldCancel)
  {
    return {histogram};
  }
  histogram.min = static_cast<float64>(minValue);
  histogram.max = static_cast<float64>(maxValue);
  detail::ConfigureHistogramBinning(histogram);
  if(histogram.binWidth <= 0.0)
  {
    histogram.counts[0] = total;
    return {histogram};
  }

  if(!compactRange)
  {
    scanBuffer.reservation = {};
    return ComputeHistogram(store, numBins, histogram.min, histogram.max, shouldCancel, options);
  }

  const float64 invBinWidth = 1.0 / histogram.binWidth;
  const int64 maxBin = static_cast<int64>(numBins) - 1;
  for(usize i = 0; i < k_RingSize; ++i)
  {
    const FrequencyEntry& entry = frequencies[i];
    if(entry.count != 0)
    {
      detail::AccumulateHistogramValue(histogram, detail::IntegralKeyAsFloat64<T>(entry.key), invBinWidth, maxBin, entry.count);
    }
  }
  return {histogram};
}

namespace detail
{
/**
 * @brief Bit-exact port of itk::Math::FloatAlmostEqual<double> (itkMath.h / itkMathDetail.h, ITK v5.4.4).
 *        Used to reproduce ITK's tie-breaking in the multi-Otsu search so the same bins are selected.
 *        The default @p maxUlps here is 1 to match the OtsuMultipleThresholdsCalculator call site.
 */
inline bool FloatAlmostEqual(float64 x1, float64 x2, int64 maxUlps = 1)
{
  // Close-to-zero guard (ITK default maxAbsoluteDifference = 0.1 * epsilon).
  constexpr float64 maxAbsoluteDifference = 0.1 * std::numeric_limits<float64>::epsilon();
  const float64 absDifference = std::abs(x1 - x2);
  if(absDifference <= maxAbsoluteDifference)
  {
    return true;
  }
  if(std::signbit(x1) != std::signbit(x2))
  {
    return false;
  }
  // Signed-magnitude -> monotonic ULP ordering (itk::Math::Detail::FloatIEEE<double>::AsULP).
  auto asULP = [](float64 f) -> int64 {
    uint64 u = 0;
    std::memcpy(&u, &f, sizeof(u));
    const bool sign = (u >> 63) != 0;
    const uint64 signMask = ~(~static_cast<uint64>(0) >> 1); // 0x8000000000000000
    int64 asInt = 0;
    std::memcpy(&asInt, &u, sizeof(asInt));
    return sign ? static_cast<int64>(signMask - u) : asInt;
  };
  int64 ulps = asULP(x1) - asULP(x2);
  if(ulps < 0)
  {
    ulps = -ulps;
  }
  return ulps <= maxUlps;
}
} // namespace detail

/**
 * @brief Bin-space multi-Otsu thresholding, a direct port of itk::OtsuMultipleThresholdsCalculator::Compute()
 *        (itkOtsuMultipleThresholdsCalculator.hxx, ITK v5.4.4) adapted to operate on the Histogram struct.
 *
 * Per-bin measurement value is the bin centroid/midpoint (min + (bin+0.5)*binWidth), matching ITK's
 * Histogram::GetMeasurementVector(bin)[0]. Returns @p numThresholds threshold values in input-intensity units.
 * Bin->value conversion uses the bin maximum (min + (bin+1)*binWidth), matching ITK's GetMaxs(), unless
 * @p returnBinMidpoint is set, in which case the bin midpoint (min + (bin+0.5)*binWidth) is used.
 */
inline std::vector<float64> OtsuMultipleThresholds(const Histogram& h, usize numThresholds, bool valleyEmphasis, bool returnBinMidpoint)
{
  const usize numBins = h.counts.size();
  std::vector<float64> thresholds;
  if(numBins == 0 || numThresholds == 0)
  {
    return thresholds;
  }
  if(numThresholds >= numBins)
  {
    // Not enough bins to place this many thresholds; the init loop below would index past the end.
    return thresholds;
  }
  float64 totalCount = 0.0;
  for(usize c : h.counts)
  {
    totalCount += static_cast<float64>(c);
  }
  if(totalCount <= 0.0)
  {
    return thresholds;
  }

  // Per-bin measurement value == bin midpoint (ITK Histogram::GetMeasurementVector(bin)[0]).
  auto measurement = [&](usize bin) -> float64 { return h.min + (static_cast<float64>(bin) + 0.5) * h.binWidth; };
  auto frequency = [&](usize bin) -> float64 { return static_cast<float64>(h.counts[bin]); };

  const usize numberOfClasses = numThresholds + 1;

  // --- Global mean (ITK: sum of measurement*frequency over all bins, normalized by total frequency) ---
  float64 globalMean = 0.0;
  for(usize b = 0; b < numBins; ++b)
  {
    globalMean += measurement(b) * frequency(b);
  }
  globalMean /= totalCount;

  // --- Initialize thresholds to the lowest configuration [0, 1, ..., numThresholds-1] ---
  std::vector<usize> thresholdIndexes(numThresholds);
  for(usize j = 0; j < numThresholds; ++j)
  {
    thresholdIndexes[j] = j;
  }
  std::vector<usize> maxVarThresholdIndexes = thresholdIndexes;

  // --- Frequency of the initial classes ---
  std::vector<float64> classFrequency(numberOfClasses, 0.0);
  float64 freqSum = 0.0;
  for(usize j = 0; j < numberOfClasses - 1; ++j)
  {
    classFrequency[j] = frequency(thresholdIndexes[j]);
    freqSum += classFrequency[j];
  }
  classFrequency[numberOfClasses - 1] = totalCount - freqSum;

  // --- Normalized histogram (PDF), used only for valley emphasis ---
  std::vector<float64> imgPDF(numBins, 0.0);
  for(usize b = 0; b < numBins; ++b)
  {
    imgPDF[b] = frequency(b) / totalCount;
  }

  // --- Mean of the initial classes ---
  std::vector<float64> classMean(numberOfClasses, 0.0);
  float64 meanSum = 0.0;
  for(usize j = 0; j < numberOfClasses - 1; ++j)
  {
    classMean[j] = (classFrequency[j] > 0.0) ? measurement(thresholdIndexes[j]) : 0.0;
    meanSum += classMean[j] * classFrequency[j];
  }
  classMean[numberOfClasses - 1] = (classFrequency[numberOfClasses - 1] > 0.0) ? (globalMean * totalCount - meanSum) / classFrequency[numberOfClasses - 1] : 0.0;

  // --- Between-class variance of the initial configuration ---
  float64 maxVarBetween = 0.0;
  for(usize j = 0; j < numberOfClasses; ++j)
  {
    maxVarBetween += classFrequency[j] * (classMean[j] * classMean[j]);
  }
  maxVarBetween /= totalCount;

  // Valley emphasis weighting of the initial configuration. NOTE: this deliberately reproduces the ITK
  // quirk where the initial factor is assigned (=) per threshold rather than accumulated (+=); only the
  // last threshold's PDF survives. The in-loop factor below uses the correct accumulation.
  if(valleyEmphasis)
  {
    float64 valleyEmphasisFactor = 0.0;
    for(usize j = 0; j < numberOfClasses - 1; ++j)
    {
      valleyEmphasisFactor = imgPDF[thresholdIndexes[j]];
    }
    valleyEmphasisFactor = 1.0 - valleyEmphasisFactor;
    maxVarBetween = maxVarBetween * valleyEmphasisFactor;
  }

  // --- IncrementThresholds: advance the odometer-style threshold configuration by one step. ---
  // Returns false when the search space is exhausted. Mutates thresholdIndexes/classMean/classFrequency.
  auto incrementThresholds = [&]() -> bool {
    // From the upper threshold down.
    for(int64 j = static_cast<int64>(numThresholds) - 1; j >= 0; --j)
    {
      const usize uj = static_cast<usize>(j);
      // Highest index this threshold may occupy (ITK: numBins - 2 - (numThresholds-1-j)); use signed math to
      // avoid unsigned underflow on tiny histograms.
      const int64 upperLimit = static_cast<int64>(numBins) - 2 - (static_cast<int64>(numThresholds) - 1 - j);
      if(static_cast<int64>(thresholdIndexes[uj]) < upperLimit)
      {
        ++thresholdIndexes[uj];

        const float64 meanOld = classMean[uj];
        const float64 freqOld = classFrequency[uj];

        classFrequency[uj] += frequency(thresholdIndexes[uj]);
        if(classFrequency[uj] > 0.0)
        {
          classMean[uj] = (meanOld * freqOld + measurement(thresholdIndexes[uj]) * frequency(thresholdIndexes[uj])) / classFrequency[uj];
        }
        else
        {
          classMean[uj] = 0.0;
        }

        // Reset higher thresholds adjacent to their predecessor and recompute their class mean/frequency.
        for(usize k = uj + 1; k < numThresholds; ++k)
        {
          thresholdIndexes[k] = thresholdIndexes[k - 1] + 1;
          classFrequency[k] = frequency(thresholdIndexes[k]);
          classMean[k] = (classFrequency[k] > 0.0) ? measurement(thresholdIndexes[k]) : 0.0;
        }

        // Recompute the highest class as the remainder.
        classFrequency[numberOfClasses - 1] = totalCount;
        classMean[numberOfClasses - 1] = globalMean * totalCount;
        for(usize k = 0; k < numberOfClasses - 1; ++k)
        {
          classFrequency[numberOfClasses - 1] -= classFrequency[k];
          classMean[numberOfClasses - 1] -= classMean[k] * classFrequency[k];
        }
        if(classFrequency[numberOfClasses - 1] > 0.0)
        {
          classMean[numberOfClasses - 1] /= classFrequency[numberOfClasses - 1];
        }
        else
        {
          classMean[numberOfClasses - 1] = 0.0;
        }

        return true; // Incremented.
      }
      if(j == 0)
      {
        return false; // Lowest threshold can't advance: search exhausted.
      }
    }
    return true;
  };

  // --- Explore every configuration; keep the one with maximum (valley-weighted) between-class variance. ---
  while(incrementThresholds())
  {
    float64 varBetween = 0.0;
    for(usize j = 0; j < numberOfClasses; ++j)
    {
      // Liao/Chen/Chung simplification of the between-class variance: sum_k omega_k * mu_k^2 (the constant
      // -mu_T^2 term is dropped since we only need the argmax).
      varBetween += classFrequency[j] * (classMean[j] * classMean[j]);
    }
    varBetween /= totalCount;

    if(valleyEmphasis)
    {
      float64 valleyEmphasisFactor = 0.0;
      for(usize j = 0; j < numberOfClasses - 1; ++j)
      {
        valleyEmphasisFactor += imgPDF[thresholdIndexes[j]];
      }
      valleyEmphasisFactor = 1.0 - valleyEmphasisFactor;
      varBetween = varBetween * valleyEmphasisFactor;
    }

    if(varBetween > maxVarBetween && !detail::FloatAlmostEqual(maxVarBetween, varBetween, 1))
    {
      maxVarBetween = varBetween;
      maxVarThresholdIndexes = thresholdIndexes;
    }
  }

  // --- Convert the selected bins to threshold values in input-intensity units. ---
  thresholds.resize(numThresholds);
  for(usize t = 0; t < numThresholds; ++t)
  {
    const float64 offset = returnBinMidpoint ? 0.5 : 1.0;
    thresholds[t] = h.min + (static_cast<float64>(maxVarThresholdIndexes[t]) + offset) * h.binWidth;
  }
  return thresholds;
}
} // namespace nx::core::ImageProcessing
