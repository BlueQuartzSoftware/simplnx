#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>
#endif

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
template <class T>
struct ArrayStatistics
{
  usize count = 0;
  T min{};
  T max{};
  float64 sum = 0.0;
  float64 sumOfSquares = 0.0;
  float64 mean = 0.0;
  float64 variance = 0.0; // sample (N-1) variance, matching itk::StatisticsImageFilter
  float64 sigma = 0.0;
};

template <class T>
struct ArrayMinMax
{
  usize count = 0;
  T min{};
  T max{};
};

namespace detail
{
inline constexpr usize k_StreamingScanFallbackBytes = 64ULL * 1024ULL * 1024ULL;
inline constexpr usize k_ArrayMinMaxBlockValues = 262144;
inline constexpr usize k_NormalizeStatisticsBlockValues = 262144;
inline constexpr usize k_ArraySumBlockValues = 262144;
inline constexpr usize k_ArraySumBlocksPerWorker = 8;

struct ExactIntegralSumBatchPlan
{
  usize batchValues = 0;
  usize blockCapacity = 0;
  usize residentBytes = 0;
};

struct ArraySumExecutionDetails
{
  bool usedExactIntegralParallel = false;
  usize blockCapacity = 0;
  usize residentBytes = 0;
};

struct ArrayMinMaxExecutionDetails
{
  bool usedFixedBlockParallel = false;
  usize blockCapacity = 0;
  usize residentBytes = 0;
};

template <class T>
struct ArrayMinMaxBlock
{
  bool hasComparableValue = false;
  T min{};
  T max{};
};

template <class T>
struct ArrayMinMaxBatchPlan
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize batchValues = 0;
  usize blockCapacity = 0;
  usize residentBytes = 0;
};

template <class T>
constexpr bool CanUseExactIntegralArraySum(usize totalValues) noexcept
{
  if constexpr(!std::is_integral_v<T>)
  {
    return false;
  }
  else
  {
    constexpr uint64 k_ExactDoubleIntegerLimit = uint64{1} << 53;
    constexpr uint64 k_AccumulatorLimit = static_cast<uint64>(std::numeric_limits<int64>::max());
    constexpr uint64 k_MaximumMagnitude = [] {
      if constexpr(std::is_signed_v<T>)
      {
        return static_cast<uint64>(std::numeric_limits<T>::max()) + uint64{1};
      }
      else
      {
        return static_cast<uint64>(std::numeric_limits<T>::max());
      }
    }();
    constexpr uint64 k_SafeTotalMagnitude = std::min(k_ExactDoubleIntegerLimit, k_AccumulatorLimit);
    if constexpr(k_MaximumMagnitude == 0)
    {
      return false;
    }
    return static_cast<uint64>(totalValues) <= k_SafeTotalMagnitude / k_MaximumMagnitude;
  }
}

inline std::optional<ExactIntegralSumBatchPlan> CreateExactIntegralSumBatchPlan(usize totalValues, usize valueBytes, usize availableBytes, usize workerCount) noexcept
{
  if(totalValues == 0 || valueBytes == 0 || availableBytes == 0)
  {
    return std::nullopt;
  }
  if(valueBytes > std::numeric_limits<usize>::max() / k_ArraySumBlockValues)
  {
    return std::nullopt;
  }
  const usize fullBlockValueBytes = k_ArraySumBlockValues * valueBytes;
  if(fullBlockValueBytes > std::numeric_limits<usize>::max() - sizeof(int64))
  {
    return std::nullopt;
  }
  const usize fullBlockResidentBytes = fullBlockValueBytes + sizeof(int64);
  const usize totalBlocks = totalValues / k_ArraySumBlockValues + static_cast<usize>((totalValues % k_ArraySumBlockValues) != 0);
  const usize boundedWorkers = std::max<usize>(1, workerCount);
  const usize desiredBlocks = boundedWorkers > std::numeric_limits<usize>::max() / k_ArraySumBlocksPerWorker ? totalBlocks : std::min(totalBlocks, boundedWorkers * k_ArraySumBlocksPerWorker);
  usize blockCapacity = std::min(desiredBlocks, availableBytes / fullBlockResidentBytes);

  // A short final block can fit even when the grant cannot hold another complete block.
  if(blockCapacity < desiredBlocks && blockCapacity + 1 == totalBlocks)
  {
    if(totalValues <= std::numeric_limits<usize>::max() / valueBytes)
    {
      const usize allValueBytes = totalValues * valueBytes;
      if(totalBlocks <= (std::numeric_limits<usize>::max() - allValueBytes) / sizeof(int64))
      {
        const usize allResidentBytes = allValueBytes + totalBlocks * sizeof(int64);
        if(allResidentBytes <= availableBytes)
        {
          ++blockCapacity;
        }
      }
    }
  }
  if(blockCapacity == 0)
  {
    return std::nullopt;
  }

  const usize batchValues = blockCapacity == totalBlocks ? totalValues : blockCapacity * k_ArraySumBlockValues;
  if(batchValues > std::numeric_limits<usize>::max() / valueBytes)
  {
    return std::nullopt;
  }
  const usize valuePayloadBytes = batchValues * valueBytes;
  if(blockCapacity > (std::numeric_limits<usize>::max() - valuePayloadBytes) / sizeof(int64))
  {
    return std::nullopt;
  }
  const usize residentBytes = valuePayloadBytes + blockCapacity * sizeof(int64);
  if(residentBytes > availableBytes)
  {
    return std::nullopt;
  }
  return ExactIntegralSumBatchPlan{.batchValues = batchValues, .blockCapacity = blockCapacity, .residentBytes = residentBytes};
}

template <class T>
struct NormalizeStatisticsBlock
{
  usize count = 0;
  T min{};
  T max{};
  float64 sum = 0.0;
  float64 sumOfSquares = 0.0;
};

template <class T>
class NormalizeStatisticsBlockAccumulator
{
public:
  void append(nonstd::span<const T> values)
  {
    for(const T value : values)
    {
      if(m_Block.count == 0)
      {
        m_Block.min = value;
        m_Block.max = value;
      }
      else
      {
        m_Block.min = std::min(m_Block.min, value);
        m_Block.max = std::max(m_Block.max, value);
      }
      ++m_Block.count;
      const float64 doubleValue = static_cast<float64>(value);
      const float64 sumValue = doubleValue - m_SumCompensation;
      const float64 nextSum = m_Block.sum + sumValue;
      m_SumCompensation = (nextSum - m_Block.sum) - sumValue;
      m_Block.sum = nextSum;
      const float64 squareValue = doubleValue * doubleValue - m_SquareCompensation;
      const float64 nextSquareSum = m_Block.sumOfSquares + squareValue;
      m_SquareCompensation = (nextSquareSum - m_Block.sumOfSquares) - squareValue;
      m_Block.sumOfSquares = nextSquareSum;
    }
  }

  [[nodiscard]] const NormalizeStatisticsBlock<T>& result() const noexcept
  {
    return m_Block;
  }

private:
  NormalizeStatisticsBlock<T> m_Block;
  float64 m_SumCompensation = 0.0;
  float64 m_SquareCompensation = 0.0;
};

template <class T>
NormalizeStatisticsBlock<T> ReduceNormalizeStatisticsBlock(nonstd::span<const T> values)
{
  NormalizeStatisticsBlockAccumulator<T> accumulator;
  accumulator.append(values);
  return accumulator.result();
}

template <class T>
void MergeNormalizeStatisticsBlock(ArrayStatistics<T>& stats, const NormalizeStatisticsBlock<T>& block, float64& sumCompensation, float64& squareCompensation)
{
  if(block.count == 0)
  {
    return;
  }
  if(stats.count == 0)
  {
    stats.min = block.min;
    stats.max = block.max;
  }
  else
  {
    stats.min = std::min(stats.min, block.min);
    stats.max = std::max(stats.max, block.max);
  }
  const float64 sumValue = block.sum - sumCompensation;
  const float64 nextSum = stats.sum + sumValue;
  sumCompensation = (nextSum - stats.sum) - sumValue;
  stats.sum = nextSum;
  const float64 squareValue = block.sumOfSquares - squareCompensation;
  const float64 nextSquareSum = stats.sumOfSquares + squareValue;
  squareCompensation = (nextSquareSum - stats.sumOfSquares) - squareValue;
  stats.sumOfSquares = nextSquareSum;
  stats.count += block.count;
}

template <class T>
void FinalizeNormalizeStatistics(ArrayStatistics<T>& stats)
{
  if(stats.count == 0)
  {
    return;
  }
  const float64 count = static_cast<float64>(stats.count);
  stats.mean = stats.sum / count;
  stats.variance = stats.count > 1 ? std::max(0.0, (stats.sumOfSquares - (stats.sum * stats.sum) / count) / (count - 1.0)) : 0.0;
  stats.sigma = std::sqrt(stats.variance);
}

struct StreamingScanBufferPlan
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize batchValues = 0;
};

template <class T>
Result<StreamingScanBufferPlan> CreateStreamingScanBufferPlan(const AbstractDataStore<T>& store, usize total = 0)
{
  if(total == 0)
  {
    total = store.getSize();
  }
  if(total == 0)
  {
    return {StreamingScanBufferPlan{}};
  }
  if(total > std::numeric_limits<usize>::max() / sizeof(T))
  {
    return MakeErrorResult<StreamingScanBufferPlan>(-8790, "Streaming statistics input byte count overflows the addressable buffer size.");
  }
  const usize usefulBytes = total * sizeof(T);
  StreamingScanBufferPlan plan;
  usize grantedBytes = std::min(usefulBytes, k_StreamingScanFallbackBytes);
  if(store.getStoreType() == IDataStore::StoreType::OutOfCore)
  {
    constexpr uint32 k_Numerator = 1;
    constexpr uint32 k_Denominator = 4;
    const uint64 preferredBytes = ResolveWorkingMemoryFractionBytes(usefulBytes, k_Numerator, k_Denominator, k_StreamingScanFallbackBytes);
    plan.reservation = ReserveWorkingMemory(preferredBytes, usefulBytes);
    if(plan.reservation.sizeBytes() == 0 || plan.reservation.sizeBytes() > std::numeric_limits<usize>::max())
    {
      return MakeErrorResult<StreamingScanBufferPlan>(-8790, "Streaming statistics could not reserve an out-of-core scan buffer.");
    }
    grantedBytes = static_cast<usize>(plan.reservation.sizeBytes());
  }
  if(grantedBytes < sizeof(T))
  {
    plan.reservation = {};
    return MakeErrorResult<StreamingScanBufferPlan>(-8790,
                                                    fmt::format("Streaming statistics received a {}-byte working-memory grant, but one {}-byte input value is required.", grantedBytes, sizeof(T)));
  }
  plan.batchValues = grantedBytes / sizeof(T);
  const usize actualBytes = plan.batchValues * sizeof(T);
  plan.reservation.shrinkTo(actualBytes);
  return {std::move(plan)};
}

template <class T>
Result<ArrayMinMaxBatchPlan<T>> CreateArrayMinMaxBatchPlan(const AbstractDataStore<T>& store, usize total = 0)
{
  if(total == 0)
  {
    total = store.getSize();
  }
  auto scanPlanResult = CreateStreamingScanBufferPlan(store, total);
  if(scanPlanResult.invalid())
  {
    return ConvertInvalidResult<ArrayMinMaxBatchPlan<T>>(std::move(scanPlanResult));
  }
  auto scanPlan = std::move(scanPlanResult.value());
  if(total == 0)
  {
    return {ArrayMinMaxBatchPlan<T>{std::move(scanPlan.reservation), 0, 0, 0}};
  }

  const usize fallbackBatchValues = std::min(total, scanPlan.batchValues);
  const uint64 reservationBytes = scanPlan.reservation.sizeBytes();
  const usize availableBytes = reservationBytes == 0 ? fallbackBatchValues * sizeof(T) : static_cast<usize>(std::min<uint64>(reservationBytes, std::numeric_limits<usize>::max()));
  if(fallbackBatchValues == 0 || availableBytes < sizeof(T))
  {
    return {ArrayMinMaxBatchPlan<T>{std::move(scanPlan.reservation), fallbackBatchValues, 0, fallbackBatchValues * sizeof(T)}};
  }

  usize batchValues = fallbackBatchValues;
  usize blockCapacity = 0;
  usize residentBytes = 0;
  for(usize iteration = 0; iteration < 4; ++iteration)
  {
    // Reserve the summary table before choosing the batch payload.
    blockCapacity = batchValues / k_ArrayMinMaxBlockValues + static_cast<usize>((batchValues % k_ArrayMinMaxBlockValues) != 0);
    if(blockCapacity < 2 || blockCapacity > std::numeric_limits<usize>::max() / sizeof(ArrayMinMaxBlock<T>))
    {
      return {ArrayMinMaxBatchPlan<T>{std::move(scanPlan.reservation), fallbackBatchValues, 0, fallbackBatchValues * sizeof(T)}};
    }
    const usize blockBytes = blockCapacity * sizeof(ArrayMinMaxBlock<T>);
    if(blockBytes >= availableBytes)
    {
      return {ArrayMinMaxBatchPlan<T>{std::move(scanPlan.reservation), fallbackBatchValues, 0, fallbackBatchValues * sizeof(T)}};
    }
    const usize nextBatchValues = std::min(total, (availableBytes - blockBytes) / sizeof(T));
    if(nextBatchValues == 0)
    {
      return {ArrayMinMaxBatchPlan<T>{std::move(scanPlan.reservation), fallbackBatchValues, 0, fallbackBatchValues * sizeof(T)}};
    }
    if(nextBatchValues == batchValues)
    {
      residentBytes = batchValues * sizeof(T) + blockBytes;
      scanPlan.reservation.shrinkTo(residentBytes);
      return {ArrayMinMaxBatchPlan<T>{std::move(scanPlan.reservation), batchValues, blockCapacity, residentBytes}};
    }
    batchValues = nextBatchValues;
  }
  return {ArrayMinMaxBatchPlan<T>{std::move(scanPlan.reservation), fallbackBatchValues, 0, fallbackBatchValues * sizeof(T)}};
}

template <class T>
ArrayMinMaxBlock<T> ReduceArrayMinMaxBlock(nonstd::span<const T> values, const std::atomic_bool& shouldCancel)
{
  ArrayMinMaxBlock<T> result;
  constexpr usize k_CancelCheckValues = 65536;
  for(usize index = 0; index < values.size(); ++index)
  {
    if((index % k_CancelCheckValues) == 0 && shouldCancel)
    {
      return result;
    }
    const T value = values[index];
    if constexpr(std::is_floating_point_v<T>)
    {
      if(std::isnan(value))
      {
        continue;
      }
    }
    if(!result.hasComparableValue)
    {
      result.hasComparableValue = true;
      result.min = value;
      result.max = value;
      continue;
    }
    if(value < result.min)
    {
      result.min = value;
    }
    if(value > result.max)
    {
      result.max = value;
    }
  }
  return result;
}

template <class T>
void MergeArrayMinMaxBlock(ArrayMinMax<T>& result, const ArrayMinMaxBlock<T>& block)
{
  if(!block.hasComparableValue)
  {
    return;
  }
  if(block.min < result.min)
  {
    result.min = block.min;
  }
  if(block.max > result.max)
  {
    result.max = block.max;
  }
}
} // namespace detail

/**
 * @brief Computes count, minimum, and maximum values.
 * @tparam T Specifies the scalar value type.
 * @param store Provides the input values.
 * @param shouldCancel Stops processing before a partial-batch merge.
 * @param executionDetails Optionally receives fixed-block execution details.
 * @return Statistics or an input-read error.
 *
 * In-memory DataStore inputs use a parallel contiguous reduction. Other stores use serial bulk reads.
 * Out-of-core workers reduce only resident blocks. A leading NaN propagates. Later NaNs are ignored.
 */
template <class T>
Result<ArrayMinMax<T>> ComputeArrayMinMax(const AbstractDataStore<T>& store, const std::atomic_bool& shouldCancel, detail::ArrayMinMaxExecutionDetails* executionDetails = nullptr)
{
  if(executionDetails != nullptr)
  {
    *executionDetails = {};
  }
  ArrayMinMax<T> result;
  const usize total = store.getSize();
  result.count = total;
  if(total == 0)
  {
    return {result};
  }

  if(const auto* inMemoryStore = dynamic_cast<const DataStore<T>*>(&store); store.getStoreType() == IDataStore::StoreType::InMemory && inMemoryStore != nullptr)
  {
    const nonstd::span<const T> values = inMemoryStore->createSpan();
    const T firstValue = values.front();
    std::pair<T, T> minMax{firstValue, firstValue};
#ifdef SIMPLNX_ENABLE_MULTICORE
    minMax = tbb::parallel_reduce(
        tbb::blocked_range<usize>(1, total), minMax,
        [&](const tbb::blocked_range<usize>& range, std::pair<T, T> local) {
          if(shouldCancel)
          {
            return local;
          }
          for(usize i = range.begin(); i < range.end(); ++i)
          {
            const T value = values[i];
            if(value < local.first)
            {
              local.first = value;
            }
            if(value > local.second)
            {
              local.second = value;
            }
          }
          return local;
        },
        [](std::pair<T, T> lhs, const std::pair<T, T>& rhs) {
          if(rhs.first < lhs.first)
          {
            lhs.first = rhs.first;
          }
          if(rhs.second > lhs.second)
          {
            lhs.second = rhs.second;
          }
          return lhs;
        });
#else
    for(usize i = 1; i < total; ++i)
    {
      if((i % 65536) == 0 && shouldCancel)
      {
        return {result};
      }
      const T value = values[i];
      if(value < minMax.first)
      {
        minMax.first = value;
      }
      if(value > minMax.second)
      {
        minMax.second = value;
      }
    }
#endif
    result.min = minMax.first;
    result.max = minMax.second;
    return {result};
  }

  auto batchPlanResult = detail::CreateArrayMinMaxBatchPlan<T>(store);
  if(batchPlanResult.invalid())
  {
    return ConvertInvalidResult<ArrayMinMax<T>>(std::move(batchPlanResult));
  }
  auto batchPlan = std::move(batchPlanResult.value());
  const usize chunkValues = std::min(batchPlan.batchValues, total);
  auto buffer = std::make_unique_for_overwrite<T[]>(chunkValues);
  std::unique_ptr<detail::ArrayMinMaxBlock<T>[]> blocks;
  if(batchPlan.blockCapacity > 0)
  {
    blocks = std::make_unique_for_overwrite<detail::ArrayMinMaxBlock<T>[]>(batchPlan.blockCapacity);
    if(executionDetails != nullptr)
    {
      executionDetails->usedFixedBlockParallel = true;
      executionDetails->blockCapacity = batchPlan.blockCapacity;
      executionDetails->residentBytes = batchPlan.residentBytes;
    }
  }
  T minValue{};
  T maxValue{};
  // Keep merged extrema private until the complete scan succeeds.
  ArrayMinMax<T> mergedMinMax;
  bool first = true;
  bool leadingNaN = false;
  for(usize start = 0; start < total; start += chunkValues)
  {
    if(shouldCancel)
    {
      return {result};
    }
    const usize count = std::min(chunkValues, total - start);
    if(Result<> readResult = store.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); readResult.invalid())
    {
      return {nonstd::make_unexpected(std::move(readResult.errors()))};
    }
    if(shouldCancel)
    {
      return {result};
    }
    usize reduceStart = 0;
    if(first)
    {
      // Seed from the exact first value to preserve leading NaNs and signed-zero ties.
      minValue = buffer[0];
      maxValue = buffer[0];
      mergedMinMax.min = minValue;
      mergedMinMax.max = maxValue;
      first = false;
      reduceStart = 1;
      if constexpr(std::is_floating_point_v<T>)
      {
        leadingNaN = std::isnan(buffer[0]);
      }
    }
    if(!leadingNaN && batchPlan.blockCapacity > 0 && reduceStart < count)
    {
      const usize reduceValues = count - reduceStart;
      const usize blockCount = reduceValues / detail::k_ArrayMinMaxBlockValues + static_cast<usize>((reduceValues % detail::k_ArrayMinMaxBlockValues) != 0);
      // Store I/O stays serial. Workers reduce separate resident blocks.
      auto reduce = [&](const Range& range) {
        for(usize blockIndex = range.min(); blockIndex < range.max(); ++blockIndex)
        {
          if(shouldCancel)
          {
            return;
          }
          const usize blockStart = reduceStart + blockIndex * detail::k_ArrayMinMaxBlockValues;
          const usize blockValues = std::min(detail::k_ArrayMinMaxBlockValues, count - blockStart);
          blocks[blockIndex] = detail::ReduceArrayMinMaxBlock<T>(nonstd::span<const T>(buffer.get() + blockStart, blockValues), shouldCancel);
        }
      };
      ParallelDataAlgorithm algorithm;
      algorithm.setRange(0, blockCount);
      algorithm.execute(reduce);
      if(shouldCancel)
      {
        return {result};
      }
      for(usize blockIndex = 0; blockIndex < blockCount; ++blockIndex)
      {
        detail::MergeArrayMinMaxBlock(mergedMinMax, blocks[blockIndex]);
      }
      minValue = mergedMinMax.min;
      maxValue = mergedMinMax.max;
      continue;
    }
    constexpr usize k_CancelCheckValues = 65536;
    for(usize blockStart = reduceStart; blockStart < count; blockStart += k_CancelCheckValues)
    {
      if(shouldCancel)
      {
        return {result};
      }
      const usize blockEnd = blockStart + std::min(k_CancelCheckValues, count - blockStart);
      for(usize i = blockStart; i < blockEnd; ++i)
      {
        const T value = buffer[i];
        if(value < minValue)
        {
          minValue = value;
        }
        if(value > maxValue)
        {
          maxValue = value;
        }
      }
    }
  }
  result.min = minValue;
  result.max = maxValue;
  return {result};
}

/**
 * @brief Single chunked bulk-I/O pass computing count/min/max (in T) and sum/sumOfSquares (in double,
 *        Kahan-compensated), then mean, sample variance ((SumSq - Sum^2/N)/(N-1)), and sigma.
 *        Bounded memory (fixed chunk buffer), so identical in-core and out-of-core.
 */
template <class T>
Result<ArrayStatistics<T>> ComputeArrayStatistics(const AbstractDataStore<T>& store, const std::atomic_bool& shouldCancel)
{
  ArrayStatistics<T> stats;
  const usize total = store.getSize();
  stats.count = total;
  if(total == 0)
  {
    return {stats};
  }

  auto bufferPlanResult = detail::CreateStreamingScanBufferPlan(store);
  if(bufferPlanResult.invalid())
  {
    return ConvertInvalidResult<ArrayStatistics<T>>(std::move(bufferPlanResult));
  }
  auto bufferPlan = std::move(bufferPlanResult.value());
  const usize chunkValues = std::min(bufferPlan.batchValues, total);
  auto buffer = std::make_unique_for_overwrite<T[]>(chunkValues);

  float64 sum = 0.0;
  float64 sumComp = 0.0; // Kahan compensation for sum
  float64 sumSq = 0.0;
  float64 sumSqComp = 0.0; // Kahan compensation for sumOfSquares
  T minValue{};
  T maxValue{};
  bool first = true;

  for(usize start = 0; start < total; start += chunkValues)
  {
    if(shouldCancel)
    {
      return {stats};
    }
    const usize count = std::min(chunkValues, total - start);
    if(Result<> readResult = store.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); readResult.invalid())
    {
      return {nonstd::make_unexpected(std::move(readResult.errors()))};
    }
    constexpr usize k_CancelCheckValues = 65536;
    for(usize blockStart = 0; blockStart < count; blockStart += k_CancelCheckValues)
    {
      if(shouldCancel)
      {
        return {stats};
      }
      const usize blockEnd = blockStart + std::min(k_CancelCheckValues, count - blockStart);
      for(usize i = blockStart; i < blockEnd; ++i)
      {
        const T v = buffer[i];
        if(first)
        {
          minValue = v;
          maxValue = v;
          first = false;
        }
        else
        {
          if(v < minValue)
          {
            minValue = v;
          }
          if(v > maxValue)
          {
            maxValue = v;
          }
        }
        const float64 dv = static_cast<float64>(v);
        // Kahan-compensated running sum of dv
        {
          const float64 y = dv - sumComp;
          const float64 t = sum + y;
          sumComp = (t - sum) - y;
          sum = t;
        }
        // Kahan-compensated running sum of dv*dv
        {
          const float64 sq = dv * dv;
          const float64 y = sq - sumSqComp;
          const float64 t = sumSq + y;
          sumSqComp = (t - sumSq) - y;
          sumSq = t;
        }
      }
    }
  }

  stats.min = minValue;
  stats.max = maxValue;
  stats.sum = sum;
  stats.sumOfSquares = sumSq;
  const float64 n = static_cast<float64>(total);
  stats.mean = sum / n;
  // Clamp to >= 0 before sqrt: the one-pass (SumSq - Sum^2/N) form can cancel a few ULPs NEGATIVE on a constant
  // or near-uniform image, which would make std::sqrt return NaN and poison sigma (and any consumer that divides
  // by it, e.g. NormalizeImageFilter). Variance is non-negative by definition, so the clamp only ever corrects
  // that floating-point cancellation artifact; a genuinely (near-)constant image correctly yields sigma == 0.
  stats.variance = (total > 1) ? std::max(0.0, (sumSq - (sum * sum) / n) / (n - 1.0)) : 0.0;
  stats.sigma = std::sqrt(stats.variance);
  return {stats};
}

template <class T>
Result<float64> ComputeArraySum(const AbstractDataStore<T>& store, const std::atomic_bool& shouldCancel, detail::ArraySumExecutionDetails* executionDetails = nullptr)
{
  if(executionDetails != nullptr)
  {
    *executionDetails = {};
  }
  const usize total = store.getSize();
  if(total == 0)
  {
    return {0.0};
  }
  auto scanPlanResult = detail::CreateStreamingScanBufferPlan(store);
  if(scanPlanResult.invalid())
  {
    return ConvertInvalidResult<float64>(std::move(scanPlanResult));
  }
  auto scanPlan = std::move(scanPlanResult.value());
  const usize serialBatchValues = std::min(total, scanPlan.batchValues);

  if constexpr(std::is_integral_v<T>)
  {
    if(detail::CanUseExactIntegralArraySum<T>(total))
    {
      const usize availableBytes =
          scanPlan.reservation.sizeBytes() == 0 ? serialBatchValues * sizeof(T) : static_cast<usize>(std::min<uint64>(scanPlan.reservation.sizeBytes(), std::numeric_limits<usize>::max()));
      const usize workerCount = std::max<usize>(1, static_cast<usize>(std::thread::hardware_concurrency()));
      const auto exactPlan = detail::CreateExactIntegralSumBatchPlan(total, sizeof(T), availableBytes, workerCount);
      if(exactPlan.has_value())
      {
        scanPlan.reservation.shrinkTo(exactPlan->residentBytes);
        auto buffer = std::make_unique_for_overwrite<T[]>(exactPlan->batchValues);
        auto blockSums = std::make_unique_for_overwrite<int64[]>(exactPlan->blockCapacity);
        if(executionDetails != nullptr)
        {
          executionDetails->usedExactIntegralParallel = true;
          executionDetails->blockCapacity = exactPlan->blockCapacity;
          executionDetails->residentBytes = exactPlan->residentBytes;
        }

        int64 exactSum = 0;
        for(usize start = 0; start < total;)
        {
          if(shouldCancel)
          {
            return {static_cast<float64>(exactSum)};
          }
          const usize count = std::min(exactPlan->batchValues, total - start);
          if(Result<> result = store.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); result.invalid())
          {
            return {nonstd::make_unexpected(std::move(result.errors()))};
          }
          if(shouldCancel)
          {
            return {static_cast<float64>(exactSum)};
          }
          const usize blockCount = count / detail::k_ArraySumBlockValues + static_cast<usize>((count % detail::k_ArraySumBlockValues) != 0);
          auto reduce = [&](const Range& range) {
            for(usize blockIndex = range.min(); blockIndex < range.max(); ++blockIndex)
            {
              if(shouldCancel)
              {
                return;
              }
              const usize blockStart = blockIndex * detail::k_ArraySumBlockValues;
              const usize blockValues = std::min(detail::k_ArraySumBlockValues, count - blockStart);
              int64 blockSum = 0;
              for(usize localIndex = 0; localIndex < blockValues; ++localIndex)
              {
                if((localIndex % 65536) == 0 && shouldCancel)
                {
                  return;
                }
                blockSum += static_cast<int64>(buffer[blockStart + localIndex]);
              }
              blockSums[blockIndex] = blockSum;
            }
          };
          ParallelDataAlgorithm algorithm;
          algorithm.setRange(0, blockCount);
          algorithm.execute(reduce);
          if(shouldCancel)
          {
            return {static_cast<float64>(exactSum)};
          }
          for(usize blockIndex = 0; blockIndex < blockCount; ++blockIndex)
          {
            exactSum += blockSums[blockIndex];
          }
          start += count;
        }
        return {static_cast<float64>(exactSum)};
      }
    }
  }

  auto buffer = std::make_unique_for_overwrite<T[]>(serialBatchValues);
  float64 sum = 0.0;
  float64 compensation = 0.0;
  for(usize start = 0; start < total;)
  {
    if(shouldCancel)
    {
      return {sum};
    }
    const usize count = std::min(serialBatchValues, total - start);
    if(Result<> result = store.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); result.invalid())
    {
      return {nonstd::make_unexpected(std::move(result.errors()))};
    }
    for(usize index = 0; index < count; ++index)
    {
      const float64 value = static_cast<float64>(buffer[index]) - compensation;
      const float64 nextSum = sum + value;
      compensation = (nextSum - sum) - value;
      sum = nextSum;
    }
    start += count;
  }
  return {sum};
}

/**
 * @brief Computes the statistics needed by NormalizeImageFilter. Exact in-memory DataStore inputs use deterministic
 *        fixed-block parallel reductions followed by a serial block-order merge. Abstract and out-of-core stores use
 *        serial bounded bulk reads, the same fixed-block reductions over resident batches, and the same merge order.
 */
template <class T>
Result<ArrayStatistics<T>> ComputeNormalizeStatistics(const AbstractDataStore<T>& store, const std::atomic_bool& shouldCancel)
{
  ArrayStatistics<T> stats;
  const usize total = store.getSize();
  if(total == 0 || shouldCancel)
  {
    return {stats};
  }
#ifdef SIMPLNX_ENABLE_MULTICORE
  if(const auto* inMemoryStore = dynamic_cast<const DataStore<T>*>(&store); store.getStoreType() == IDataStore::StoreType::InMemory && inMemoryStore != nullptr)
  {
    const usize blockCount = total / detail::k_NormalizeStatisticsBlockValues + static_cast<usize>((total % detail::k_NormalizeStatisticsBlockValues) != 0);
    std::vector<detail::NormalizeStatisticsBlock<T>> blocks(blockCount);
    const nonstd::span<const T> values = inMemoryStore->createSpan();
    auto reduceBlocks = [&](const Range& range) {
      for(usize blockIndex = range.min(); blockIndex < range.max(); ++blockIndex)
      {
        if(shouldCancel)
        {
          return;
        }
        const usize start = blockIndex * detail::k_NormalizeStatisticsBlockValues;
        const usize count = std::min(detail::k_NormalizeStatisticsBlockValues, total - start);
        blocks[blockIndex] = detail::ReduceNormalizeStatisticsBlock<T>(nonstd::span<const T>(values.data() + start, count));
      }
    };
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, blockCount);
    parallelAlgorithm.execute(reduceBlocks);
    if(shouldCancel)
    {
      return {stats};
    }

    float64 sumCompensation = 0.0;
    float64 squareCompensation = 0.0;
    for(const detail::NormalizeStatisticsBlock<T>& block : blocks)
    {
      detail::MergeNormalizeStatisticsBlock(stats, block, sumCompensation, squareCompensation);
    }
    detail::FinalizeNormalizeStatistics(stats);
    return {stats};
  }
#endif
  auto planResult = detail::CreateStreamingScanBufferPlan(store);
  if(planResult.invalid())
  {
    return ConvertInvalidResult<ArrayStatistics<T>>(std::move(planResult));
  }
  auto plan = std::move(planResult.value());
  const usize transportValues = std::min(plan.batchValues, total);
  if(detail::k_NormalizeStatisticsBlockValues > std::numeric_limits<usize>::max() / sizeof(T))
  {
    return MakeErrorResult<ArrayStatistics<T>>(-8791, fmt::format("Normalize statistics block size overflows: {} values at {} bytes each.", detail::k_NormalizeStatisticsBlockValues, sizeof(T)));
  }
  const usize blockValueBytes = detail::k_NormalizeStatisticsBlockValues * sizeof(T);
  if(blockValueBytes > std::numeric_limits<usize>::max() - sizeof(detail::NormalizeStatisticsBlock<T>))
  {
    return MakeErrorResult<ArrayStatistics<T>>(
        -8791, fmt::format("Normalize statistics resident block size overflows: {} value bytes plus {} result bytes.", blockValueBytes, sizeof(detail::NormalizeStatisticsBlock<T>)));
  }
  const usize residentBytesPerBlock = blockValueBytes + sizeof(detail::NormalizeStatisticsBlock<T>);
  const uint64 reservationBytes = plan.reservation.sizeBytes();
  const usize availableBytes = reservationBytes > 0 ? static_cast<usize>(std::min<uint64>(reservationBytes, std::numeric_limits<usize>::max())) : transportValues * sizeof(T);
  const usize grantedResidentBlocks = availableBytes / residentBytesPerBlock;
  const usize workerBlocks = std::max<usize>(1, static_cast<usize>(std::thread::hardware_concurrency()));
  const usize totalBlocks = total / detail::k_NormalizeStatisticsBlockValues + static_cast<usize>((total % detail::k_NormalizeStatisticsBlockValues) != 0);
  const usize batchBlocks = std::min({grantedResidentBlocks, workerBlocks, totalBlocks});

  float64 sumCompensation = 0.0;
  float64 squareCompensation = 0.0;
  if(batchBlocks == 0)
  {
    const usize bufferValues = std::max<usize>(1, transportValues);
    plan.reservation.shrinkTo(static_cast<uint64>(bufferValues * sizeof(T)));
    auto buffer = std::make_unique_for_overwrite<T[]>(bufferValues);
    usize blockStart = 0;
    while(blockStart < total)
    {
      if(shouldCancel)
      {
        return {stats};
      }
      const usize logicalBlockValues = std::min(detail::k_NormalizeStatisticsBlockValues, total - blockStart);
      detail::NormalizeStatisticsBlockAccumulator<T> accumulator;
      usize consumedValues = 0;
      while(consumedValues < logicalBlockValues)
      {
        if(shouldCancel)
        {
          return {stats};
        }
        const usize readValues = std::min(bufferValues, logicalBlockValues - consumedValues);
        if(Result<> result = store.copyIntoBuffer(blockStart + consumedValues, nonstd::span<T>(buffer.get(), readValues)); result.invalid())
        {
          return {nonstd::make_unexpected(std::move(result.errors()))};
        }
        if(shouldCancel)
        {
          return {stats};
        }
        accumulator.append(nonstd::span<const T>(buffer.get(), readValues));
        consumedValues += readValues;
      }
      detail::MergeNormalizeStatisticsBlock(stats, accumulator.result(), sumCompensation, squareCompensation);
      blockStart += logicalBlockValues;
    }
    detail::FinalizeNormalizeStatistics(stats);
    return {stats};
  }

  const usize batchValues = batchBlocks * detail::k_NormalizeStatisticsBlockValues;
  const usize residentPayloadBytes = batchValues * sizeof(T) + batchBlocks * sizeof(detail::NormalizeStatisticsBlock<T>);
  plan.reservation.shrinkTo(static_cast<uint64>(residentPayloadBytes));
  auto buffer = std::make_unique_for_overwrite<T[]>(batchValues);
  std::vector<detail::NormalizeStatisticsBlock<T>> blocks(batchBlocks);
  usize batchStart = 0;
  while(batchStart < total)
  {
    if(shouldCancel)
    {
      return {stats};
    }
    const usize count = std::min(batchValues, total - batchStart);
    if(Result<> result = store.copyIntoBuffer(batchStart, nonstd::span<T>(buffer.get(), count)); result.invalid())
    {
      return {nonstd::make_unexpected(std::move(result.errors()))};
    }
    if(shouldCancel)
    {
      return {stats};
    }
    const usize localBlockCount = count / detail::k_NormalizeStatisticsBlockValues + static_cast<usize>((count % detail::k_NormalizeStatisticsBlockValues) != 0);
    auto reduce = [&](const Range& range) {
      for(usize localBlock = range.min(); localBlock < range.max(); ++localBlock)
      {
        if(shouldCancel)
        {
          return;
        }
        const usize start = localBlock * detail::k_NormalizeStatisticsBlockValues;
        const usize blockValues = std::min(detail::k_NormalizeStatisticsBlockValues, count - start);
        blocks[localBlock] = detail::ReduceNormalizeStatisticsBlock<T>(nonstd::span<const T>(buffer.get() + start, blockValues));
      }
    };
    ParallelDataAlgorithm algorithm;
    algorithm.setRange(0, localBlockCount);
    algorithm.execute(reduce);
    if(shouldCancel)
    {
      return {stats};
    }
    for(usize localBlock = 0; localBlock < localBlockCount; ++localBlock)
    {
      detail::MergeNormalizeStatisticsBlock(stats, blocks[localBlock], sumCompensation, squareCompensation);
    }
    batchStart += count;
  }
  detail::FinalizeNormalizeStatistics(stats);
  return {stats};
}
} // namespace nx::core::ImageProcessing
