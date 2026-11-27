#include "ComputeArrayStatistics.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/IExternalSort.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/HistogramUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/Math/StatisticsCalculations.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
#ifdef SIMPLNX_ENABLE_MULTICORE
/**
 * @brief Checks whether supplied arrays avoid out-of-core storage.
 * @param arrays Arrays used by one parallel operation.
 * @return True when no supplied array is out of core.
 *
 * Resident statistics use parallel random access. Out-of-core stores require
 * bounded bulk I/O and must not enter that concurrent path.
 */
bool CheckArraysInMemory(const nx::core::IParallelAlgorithm::AlgorithmArraysType& arrays)
{
  if(arrays.empty())
  {
    return true;
  }

  for(const auto* arrayPtr : arrays)
  {
    if(arrayPtr == nullptr)
    {
      continue;
    }

    if(arrayPtr->getIDataStoreRef().getStoreType() == nx::core::IDataStore::StoreType::OutOfCore)
    {
      return false;
    }
  }

  return true;
}
#endif

/**
 * @class StatisticsByFeatureImpl
 * @brief Computes resident statistics for one parallel feature range.
 * @tparam T Input and extrema value type.
 *
 * This implementation operates only on in-memory stores. Each worker owns a
 * feature range and borrows all arrays for synchronous execution.
 */
template <typename T>
class StatisticsByFeatureImpl
{
public:
  /**
   * @brief Creates a resident feature-statistics worker.
   * @param length True to write sample counts.
   * @param min True to write minima.
   * @param max True to write maxima.
   * @param mean True to write means.
   * @param mode True to write modes.
   * @param stdDeviation True to write standard deviations.
   * @param summation True to write sums.
   * @param mask Selects accepted tuples.
   * @param featureIds Maps tuples to features.
   * @param source Input values.
   * @param featureHasDataArray Receives feature availability.
   * @param lengthArray Receives sample counts.
   * @param minArray Receives minima.
   * @param maxArray Receives maxima.
   * @param meanArray Receives means.
   * @param modeArray Receives modes.
   * @param stdDevArray Receives standard deviations.
   * @param summationArray Receives sums.
   * @param shouldCancel Cancellation flag.
   * @param algorithm Serializes progress and status messages.
   * @pre Referenced arrays, mask, and algorithm outlive this worker.
   */
  StatisticsByFeatureImpl(bool length, bool min, bool max, bool mean, bool mode, bool stdDeviation, bool summation, const std::unique_ptr<mask_compare_utilities::MaskCompare>& mask,
                          const Int32AbstractDataStoreType& featureIds, const AbstractDataStore<T>& source, BoolArrayType* featureHasDataArray, UInt64ArrayType* lengthArray, DataArray<T>* minArray,
                          DataArray<T>* maxArray, Float32ArrayType* meanArray, NeighborList<T>* modeArray, Float32ArrayType* stdDevArray, Float32ArrayType* summationArray, const std::atomic_bool& shouldCancel,
                          ComputeArrayStatistics& algorithm)
  : m_Length(length)
  , m_Min(min)
  , m_Max(max)
  , m_Mean(mean)
  , m_Mode(mode)
  , m_StdDeviation(stdDeviation)
  , m_Summation(summation)
  , m_Mask(mask)
  , m_FeatureIds(featureIds)
  , m_Source(source)
  , m_FeatureHasDataArray(featureHasDataArray)
  , m_LengthArray(lengthArray)
  , m_MinArray(minArray)
  , m_MaxArray(maxArray)
  , m_MeanArray(meanArray)
  , m_ModeArray(modeArray)
  , m_StdDevArray(stdDevArray)
  , m_SummationArray(summationArray)
  , m_ShouldCancel(shouldCancel)
  , m_Algorithm(algorithm)
  {
  }

  /**
   * @brief Computes statistics for an inclusive-exclusive feature range.
   * @param start First feature identifier.
   * @param end One past the last feature identifier.
   *
   * Standard deviation uses a second resident input pass after means exist.
   */
  void compute(usize start, usize end) const
  {
    const usize numTuples = m_FeatureIds.getNumberOfTuples();
    const usize numCurrentFeatures = end - start;

    const auto reportProgress = [this, numCurrentFeatures](usize completedTuples) { m_Algorithm.sendThreadSafeProgressMessage(completedTuples * numCurrentFeatures); };
    auto [length, min, max, summation, modalMaps] = histogram_utilities::concurrent::CalculateFeatureHasDataStats(m_Source, m_FeatureIds, start, end, m_Mask, reportProgress, m_ShouldCancel);
    if(m_ShouldCancel)
    {
      return;
    }

    usize progressCount = 0;
    constexpr usize k_ProgressChunkSize = 4096;

    m_Algorithm.sendThreadSafeInfoMessage(fmt::format("Calculating statistics for feature range [{}-{}]", start, end));

    std::vector<float32> meanArray;
    if(m_StdDeviation && !m_Mean)
    {
      meanArray.resize(numCurrentFeatures);
    }
    for(usize j = start; j < end; j++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      const usize localFeatureIndex = j - start;

      m_FeatureHasDataArray->initializeTuple(j, (length[localFeatureIndex] > 0));
      if(m_Length)
      {
        m_LengthArray->initializeTuple(j, length[localFeatureIndex]);
      }
      if(m_Min)
      {
        m_MinArray->initializeTuple(j, min[localFeatureIndex]);
      }
      if(m_Max)
      {
        m_MaxArray->initializeTuple(j, max[localFeatureIndex]);
      }
      if(m_Summation)
      {
        m_SummationArray->initializeTuple(j, summation[localFeatureIndex]);
      }

      float32 meanValue = 0.0f;
      if(length[localFeatureIndex] > 0)
      {
        meanValue = summation[localFeatureIndex] / static_cast<float32>(length[localFeatureIndex]);
      }

      if(m_Mean)
      {
        m_MeanArray->initializeTuple(j, meanValue);
      }
      else if(m_StdDeviation)
      {
        meanArray[localFeatureIndex] = meanValue;
      }

      if constexpr(!std::is_same_v<T, bool>)
      {
        if(m_Mode && !modalMaps[localFeatureIndex].empty())
        {
          auto pr = std::max_element(modalMaps[localFeatureIndex].begin(), modalMaps[localFeatureIndex].end(), [](const auto& x, const auto& y) { return x.second < y.second; });
          int maxCount = pr->second;

          for(const auto& modalPair : modalMaps[localFeatureIndex])
          {
            if(modalPair.second == maxCount)
            {
              m_ModeArray->addEntry(j, modalPair.first);
            }
          }
        }
      }

      progressCount++;
      if(progressCount == k_ProgressChunkSize || j + 1 == end)
      {
        m_Algorithm.sendThreadSafeProgressMessage(progressCount);
        progressCount = 0;
      }
    }

    if(m_StdDeviation)
    {
      m_Algorithm.sendThreadSafeInfoMessage(fmt::format("Computing StdDev Feature/Ensemble [{}-{}]", start, end));
      // Float64 accumulators reduce rounding loss before Float32 standard-deviation output.
      std::vector<float64> sumOfDiffs(numCurrentFeatures, 0.0f);
      for(usize chunkStart = 0; chunkStart < numTuples; chunkStart += k_ProgressChunkSize)
      {
        if(m_ShouldCancel)
        {
          return;
        }
        const usize chunkEnd = std::min(chunkStart + k_ProgressChunkSize, numTuples);
        for(usize tupleIndex = chunkStart; tupleIndex < chunkEnd; tupleIndex++)
        {
          if(m_Mask != nullptr && !m_Mask->isTrue(tupleIndex))
          {
            continue;
          }
          const int32 featureId = m_FeatureIds[tupleIndex];
          if(featureId < start || featureId >= end)
          {
            continue;
          }

          const float32 meanVal = m_Mean ? m_MeanArray->operator[](featureId) : meanArray[featureId - start];
          sumOfDiffs[featureId - start] += static_cast<float64>((m_Source[tupleIndex] - meanVal) * (m_Source[tupleIndex] - meanVal));
        }
        reportProgress(chunkEnd - chunkStart);
      }

      for(usize j = 0; j < numCurrentFeatures; j++)
      {
        const uint64 lengthVal = m_Length ? m_LengthArray->operator[](j + start) : length[j];
        if(lengthVal > 0)
        {
          m_StdDevArray->operator[](j + start) = static_cast<float32>(std::sqrt(sumOfDiffs[j] / static_cast<float64>(lengthVal)));
        }
      }
    }
  }

  /**
   * @brief Computes one parallel feature range.
   * @param range Inclusive-exclusive feature range.
   */
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  bool m_Length;
  bool m_Min;
  bool m_Max;
  bool m_Mean;
  bool m_Mode;
  bool m_StdDeviation;
  bool m_Summation;
  const std::unique_ptr<mask_compare_utilities::MaskCompare>& m_Mask = nullptr;
  const Int32AbstractDataStoreType& m_FeatureIds;
  const AbstractDataStore<T>& m_Source;
  BoolArrayType* m_FeatureHasDataArray = nullptr;
  UInt64ArrayType* m_LengthArray = nullptr;
  DataArray<T>* m_MinArray = nullptr;
  DataArray<T>* m_MaxArray = nullptr;
  Float32ArrayType* m_MeanArray = nullptr;
  NeighborList<T>* m_ModeArray = nullptr;
  Float32ArrayType* m_StdDevArray = nullptr;
  Float32ArrayType* m_SummationArray = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  ComputeArrayStatistics& m_Algorithm;
};

constexpr usize k_ChunkTuples = 65536; // Bounds each cell-level bulk-I/O page.

/**
 * @class StatisticsByFeatureRangeImpl
 * @brief Computes bounded statistics for a contiguous feature range.
 * @tparam T Input and extrema value type.
 *
 * One page scan routes each tuple to a feature accumulator. Standard deviation
 * requires a second page scan after means are known. Cell buffers stay fixed;
 * accumulators scale with feature count rather than cell count.
 */
template <typename T>
class StatisticsByFeatureRangeImpl
{
public:
  /**
   * @brief Creates a bounded feature-statistics operation.
   * @param length True to write sample counts.
   * @param min True to write minima.
   * @param max True to write maxima.
   * @param mean True to write means.
   * @param mode True to write modes.
   * @param stdDeviation True to write standard deviations.
   * @param summation True to write sums.
   * @param featureIdToCompactIndex Maps feature IDs to output indices.
   * @param mask Selects accepted tuples.
   * @param featureIds Maps tuples to features.
   * @param source Input values.
   * @param start First feature identifier.
   * @param numFeatures Number of output features.
   * @param featureHasDataArray Receives feature availability.
   * @param lengthArray Receives sample counts.
   * @param minArray Receives minima.
   * @param maxArray Receives maxima.
   * @param meanArray Receives means.
   * @param modeArray Receives modes.
   * @param stdDevArray Receives standard deviations.
   * @param summationArray Receives sums.
   * @param shouldCancel Cancellation flag.
   * @pre Referenced arrays, map, mask, and outputs outlive this operation.
   */
  StatisticsByFeatureRangeImpl(bool length, bool min, bool max, bool mean, bool mode, bool stdDeviation, bool summation, const std::vector<usize>& featureIdToCompactIndex,
                               const std::unique_ptr<mask_compare_utilities::MaskCompare>& mask, const Int32AbstractDataStoreType& featureIds, const AbstractDataStore<T>& source, int32 start,
                               usize numFeatures, BoolArrayType* featureHasDataArray, UInt64ArrayType* lengthArray, DataArray<T>* minArray, DataArray<T>* maxArray, Float32ArrayType* meanArray,
                               NeighborList<T>* modeArray, Float32ArrayType* stdDevArray, Float32ArrayType* summationArray, const std::atomic_bool& shouldCancel)
  : m_Length(length)
  , m_Min(min)
  , m_Max(max)
  , m_Mean(mean)
  , m_Mode(mode)
  , m_StdDeviation(stdDeviation)
  , m_Summation(summation)
  , m_FeatureIdToCompactIndex(featureIdToCompactIndex)
  , m_Mask(mask)
  , m_FeatureIds(featureIds)
  , m_Source(source)
  , m_Start(start)
  , m_NumFeatures(numFeatures)
  , m_FeatureHasDataArray(featureHasDataArray)
  , m_LengthArray(lengthArray)
  , m_MinArray(minArray)
  , m_MaxArray(maxArray)
  , m_MeanArray(meanArray)
  , m_ModeArray(modeArray)
  , m_StdDevArray(stdDevArray)
  , m_SummationArray(summationArray)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Runs bounded statistics for the configured feature range.
   * @return Bulk-I/O or cancellation result.
   *
   * Output writes follow successful page scans. Empty features keep initialized
   * sentinel values for statistics that have no sample.
   */
  Result<> operator()() const
  {
    const usize numTuples = m_Source.getNumberOfTuples();

    // Feature-scale accumulators stay resident. T arrays avoid vector<bool> proxy arithmetic.
    std::vector<usize> counts(m_NumFeatures, 0);
    auto minValues = std::make_unique<T[]>(m_NumFeatures);
    auto maxValues = std::make_unique<T[]>(m_NumFeatures);
    auto sums = std::make_unique<T[]>(m_NumFeatures);
    for(usize k = 0; k < m_NumFeatures; k++)
    {
      minValues[k] = std::numeric_limits<T>::max();
      if constexpr(std::is_floating_point_v<T>)
      {
        maxValues[k] = -std::numeric_limits<T>::max();
      }
      else
      {
        maxValues[k] = std::numeric_limits<T>::min();
      }
      sums[k] = static_cast<T>(0);
    }
    std::vector<std::map<T, uint64>> modalMaps(m_Mode ? m_NumFeatures : 0);

    auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
    auto sourceBuf = std::make_unique<T[]>(k_ChunkTuples);

    // One page scan collects counts, extrema, sums, and optional mode frequencies.
    for(usize offset = 0; offset < numTuples; offset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize chunkCount = std::min(k_ChunkTuples, numTuples - offset);

      Result<> featureIdResult = m_FeatureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), chunkCount));
      if(featureIdResult.invalid())
      {
        return featureIdResult;
      }
      Result<> sourceResult = m_Source.copyIntoBuffer(offset, nonstd::span<T>(sourceBuf.get(), chunkCount));
      if(sourceResult.invalid())
      {
        return sourceResult;
      }

      for(usize idx = 0; idx < chunkCount; idx++)
      {
        const int32 featureId = featureIdBuf[idx];
        if((m_Mask != nullptr && !m_Mask->isTrue(offset + idx)) || featureId < m_Start || static_cast<int64>(featureId) >= static_cast<int64>(m_Start) + static_cast<int64>(m_NumFeatures))
        {
          continue;
        }
        const usize compactIndex = m_FeatureIdToCompactIndex[static_cast<usize>(static_cast<int64>(featureId) - static_cast<int64>(m_Start))];
        const T val = sourceBuf[idx];

        counts[compactIndex]++;
        if(val < minValues[compactIndex])
        {
          minValues[compactIndex] = val;
        }
        if(val > maxValues[compactIndex])
        {
          maxValues[compactIndex] = val;
        }
        sums[compactIndex] += val;
        if(m_Mode)
        {
          modalMaps[compactIndex][val]++;
        }
      }
    }

    // Standard deviation needs means even when Mean output is disabled.
    std::vector<float32> meanValues(m_NumFeatures, 0.0f);
    for(usize k = 0; k < m_NumFeatures; k++)
    {
      if(counts[k] == 0)
      {
        continue;
      }
      meanValues[k] = static_cast<float32>(sums[k]) / static_cast<float32>(counts[k]);
    }

    // Variance requires completed means, so standard deviation uses a second page scan.
    std::vector<float64> sumOfDiffs(m_StdDeviation ? m_NumFeatures : 0, 0.0);
    if(m_StdDeviation)
    {
      for(usize offset = 0; offset < numTuples; offset += k_ChunkTuples)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize chunkCount = std::min(k_ChunkTuples, numTuples - offset);

        Result<> featureIdResult = m_FeatureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), chunkCount));
        if(featureIdResult.invalid())
        {
          return featureIdResult;
        }
        Result<> sourceResult = m_Source.copyIntoBuffer(offset, nonstd::span<T>(sourceBuf.get(), chunkCount));
        if(sourceResult.invalid())
        {
          return sourceResult;
        }

        for(usize idx = 0; idx < chunkCount; idx++)
        {
          const int32 featureId = featureIdBuf[idx];
          if((m_Mask != nullptr && !m_Mask->isTrue(offset + idx)) || featureId < m_Start || static_cast<int64>(featureId) >= static_cast<int64>(m_Start) + static_cast<int64>(m_NumFeatures))
          {
            continue;
          }
          const usize compactIndex = m_FeatureIdToCompactIndex[static_cast<usize>(static_cast<int64>(featureId) - static_cast<int64>(m_Start))];
          const float32 meanVal = meanValues[compactIndex];
          sumOfDiffs[compactIndex] += static_cast<float64>((sourceBuf[idx] - meanVal) * (sourceBuf[idx] - meanVal));
        }
      }
    }

    // Empty features retain the sentinel values established during initialization.
    for(usize k = 0; k < m_NumFeatures; k++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const bool hasData = counts[k] > 0;
      m_FeatureHasDataArray->initializeTuple(k, hasData);
      if(!hasData)
      {
        continue;
      }

      if(m_Length)
      {
        m_LengthArray->initializeTuple(k, static_cast<uint64>(counts[k]));
      }
      if(m_Min)
      {
        m_MinArray->initializeTuple(k, minValues[k]);
      }
      if(m_Max)
      {
        m_MaxArray->initializeTuple(k, maxValues[k]);
      }
      if(m_Summation)
      {
        m_SummationArray->initializeTuple(k, sums[k]);
      }
      if(m_Mean)
      {
        m_MeanArray->initializeTuple(k, meanValues[k]);
      }
      if constexpr(!std::is_same_v<T, bool>)
      {
        if(m_Mode && !modalMaps[k].empty())
        {
          auto pr = std::max_element(modalMaps[k].begin(), modalMaps[k].end(), [](const auto& x, const auto& y) { return x.second < y.second; });
          const uint64 maxCount = pr->second;

          for(const auto& modalPair : modalMaps[k])
          {
            if(modalPair.second == maxCount)
            {
              m_ModeArray->addEntry(k, modalPair.first);
            }
          }
        }
      }
      if(m_StdDeviation)
      {
        m_StdDevArray->setValue(k, static_cast<float32>(std::sqrt(sumOfDiffs[k] / static_cast<float64>(counts[k]))));
      }
    }

    return {};
  }

private:
  bool m_Length;
  bool m_Min;
  bool m_Max;
  bool m_Mean;
  bool m_Mode;
  bool m_StdDeviation;
  bool m_Summation;
  const std::vector<usize>& m_FeatureIdToCompactIndex;
  const std::unique_ptr<mask_compare_utilities::MaskCompare>& m_Mask;
  const Int32AbstractDataStoreType& m_FeatureIds;
  const AbstractDataStore<T>& m_Source;
  int32 m_Start;
  usize m_NumFeatures;
  BoolArrayType* m_FeatureHasDataArray = nullptr;
  UInt64ArrayType* m_LengthArray = nullptr;
  DataArray<T>* m_MinArray = nullptr;
  DataArray<T>* m_MaxArray = nullptr;
  Float32ArrayType* m_MeanArray = nullptr;
  NeighborList<T>* m_ModeArray = nullptr;
  Float32ArrayType* m_StdDevArray = nullptr;
  Float32ArrayType* m_SummationArray = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class MedianByFeatureImpl
 * @brief Computes resident exact medians and unique counts by feature.
 * @tparam T Input value type.
 *
 * Exact median materializes selected values per feature. This resident-only
 * implementation runs after the caller requires all participating arrays in memory.
 */
template <typename T>
class MedianByFeatureImpl
{
public:
  /**
   * @brief Creates a resident median and unique-count worker.
   * @param mask Selects accepted tuples.
   * @param featureIds Maps tuples to features.
   * @param source Input values.
   * @param findMedian True to calculate medians.
   * @param findNumUnique True to calculate unique counts.
   * @param medianArray Receives medians.
   * @param numUniqueValuesArray Receives unique counts.
   * @param lengthArray Supplies feature capacities.
   * @param algorithm Serializes progress and status messages.
   * @pre Referenced arrays, mask, and algorithm outlive this worker.
   */
  MedianByFeatureImpl(const std::unique_ptr<mask_compare_utilities::MaskCompare>& mask, const Int32AbstractDataStoreType& featureIds, const AbstractDataStore<T>& source, bool findMedian, bool findNumUnique,
                      Float32ArrayType* medianArray, Int32ArrayType* numUniqueValuesArray, DataArray<uint64>* lengthArray, ComputeArrayStatistics& algorithm)
  : m_FindMedian(findMedian)
  , m_FindNumUniqueValues(findNumUnique)
  , m_MedianArray(medianArray)
  , m_NumUniqueValuesArray(numUniqueValuesArray)
  , m_Mask(mask)
  , m_FeatureIds(featureIds)
  , m_Source(source)
  , m_LengthArray(lengthArray)
  , m_Algorithm(algorithm)
  {
  }

  /**
   * @brief Computes resident medians and unique counts for a feature range.
   * @param start First feature identifier.
   * @param end One past the last feature identifier.
   */
  void compute(usize start, usize end) const
  {
    m_Algorithm.sendThreadSafeInfoMessage(fmt::format("Starting Median Array Calculation: Feature/Ensemble [{}-{}]", start, end));

    const usize numFeatureSources = end - start;
    // Reserve known feature capacities before collecting exact values.
    std::vector<std::vector<T>> featureSources(numFeatureSources);
    for(usize featureSourceIndex = 0; featureSourceIndex < numFeatureSources; featureSourceIndex++)
    {
      if(m_LengthArray != nullptr)
      {
        featureSources[featureSourceIndex].reserve(m_LengthArray->operator[](featureSourceIndex + start));
      }
    }
    const usize numTuples = m_Source.getNumberOfTuples();

    for(usize tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
    {
      if(m_Mask != nullptr && !m_Mask->isTrue(tupleIndex))
      {
        continue;
      }
      const int32 featureId = m_FeatureIds[tupleIndex];
      if(featureId < start || featureId >= end)
      {
        continue;
      }
      featureSources[featureId - start].push_back(m_Source[tupleIndex]);
    }

    for(usize featureSourceIndex = 0; featureSourceIndex < numFeatureSources; featureSourceIndex++)
    {
      if(m_FindMedian)
      {
        const float32 val = statistics_calculations::FindMedian(featureSources[featureSourceIndex]);
        m_MedianArray->setValue(featureSourceIndex + start, val);
      }
      if(m_FindNumUniqueValues)
      {
        const auto val = statistics_calculations::FindNumUniqueValues(featureSources[featureSourceIndex]);
        m_NumUniqueValuesArray->setValue(featureSourceIndex + start, val);
      }
    }
  }

  /**
   * @brief Computes one parallel feature range.
   * @param range Inclusive-exclusive feature range.
   */
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  bool m_FindMedian;
  bool m_FindNumUniqueValues;
  Float32ArrayType* m_MedianArray;
  Int32ArrayType* m_NumUniqueValuesArray;
  const std::unique_ptr<mask_compare_utilities::MaskCompare>& m_Mask = nullptr;
  const Int32AbstractDataStoreType& m_FeatureIds;
  const AbstractDataStore<T>& m_Source;
  const DataArray<uint64>* m_LengthArray = nullptr;
  ComputeArrayStatistics& m_Algorithm;
};

/**
 * @class MedianByFeatureRangeImpl
 * @brief Computes resident exact medians and unique counts for a feature range.
 * @tparam T Input value type.
 *
 * One bulk page scan buckets each in-range value once. Exact median requires
 * values retained per feature, so memory grows with selected cells. The bounded
 * scanline implementation avoids this resident collection.
 */
template <typename T>
class MedianByFeatureRangeImpl
{
public:
  /**
   * @brief Creates a resident range median and unique-count operation.
   * @param featureIdToCompactIndex Maps feature IDs to output indices.
   * @param mask Selects accepted tuples.
   * @param featureIds Maps tuples to features.
   * @param source Input values.
   * @param start First feature identifier.
   * @param numFeatures Number of output features.
   * @param findMedian True to calculate medians.
   * @param findNumUnique True to calculate unique counts.
   * @param medianArray Receives medians.
   * @param numUniqueValuesArray Receives unique counts.
   * @param shouldCancel Cancellation flag.
   * @pre Referenced arrays, map, and mask outlive this operation.
   */
  MedianByFeatureRangeImpl(const std::vector<usize>& featureIdToCompactIndex, const std::unique_ptr<mask_compare_utilities::MaskCompare>& mask, const Int32AbstractDataStoreType& featureIds,
                           const AbstractDataStore<T>& source, int32 start, usize numFeatures, bool findMedian, bool findNumUnique, Float32ArrayType* medianArray, Int32ArrayType* numUniqueValuesArray,
                           const std::atomic_bool& shouldCancel)
  : m_FindMedian(findMedian)
  , m_FindNumUniqueValues(findNumUnique)
  , m_MedianArray(medianArray)
  , m_NumUniqueValuesArray(numUniqueValuesArray)
  , m_FeatureIdToCompactIndex(featureIdToCompactIndex)
  , m_Mask(mask)
  , m_FeatureIds(featureIds)
  , m_Source(source)
  , m_Start(start)
  , m_NumFeatures(numFeatures)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Runs resident exact median and unique-value bucketing.
   * @return Bulk-I/O or cancellation result.
   */
  Result<> operator()() const
  {
    const usize numTuples = m_Source.getNumberOfTuples();
    // Exact median retains selected values in per-feature resident buffers.
    std::vector<std::vector<T>> perFeatureValues(m_FindMedian ? m_NumFeatures : 0);
    std::vector<std::set<T>> perFeatureUniqueValues(m_FindNumUniqueValues ? m_NumFeatures : 0);

    auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
    auto sourceBuf = std::make_unique<T[]>(k_ChunkTuples);

    for(usize offset = 0; offset < numTuples; offset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize chunkCount = std::min(k_ChunkTuples, numTuples - offset);

      Result<> featureIdResult = m_FeatureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), chunkCount));
      if(featureIdResult.invalid())
      {
        return featureIdResult;
      }
      Result<> sourceResult = m_Source.copyIntoBuffer(offset, nonstd::span<T>(sourceBuf.get(), chunkCount));
      if(sourceResult.invalid())
      {
        return sourceResult;
      }

      for(usize idx = 0; idx < chunkCount; idx++)
      {
        const int32 featureId = featureIdBuf[idx];
        if((m_Mask != nullptr && !m_Mask->isTrue(offset + idx)) || featureId < m_Start || static_cast<int64>(featureId) >= static_cast<int64>(m_Start) + static_cast<int64>(m_NumFeatures))
        {
          continue;
        }
        const usize compactIndex = m_FeatureIdToCompactIndex[static_cast<usize>(static_cast<int64>(featureId) - static_cast<int64>(m_Start))];
        if(m_FindMedian)
        {
          perFeatureValues[compactIndex].push_back(sourceBuf[idx]);
        }
        if(m_FindNumUniqueValues)
        {
          perFeatureUniqueValues[compactIndex].emplace(sourceBuf[idx]);
        }
      }
    }

    for(usize k = 0; k < m_NumFeatures; k++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      if(m_FindMedian)
      {
        auto& values = perFeatureValues[k];
        if(values.empty())
        {
          m_MedianArray->setValue(k, 0.0f);
        }
        else
        {
          std::sort(values.begin(), values.end());
          if(values.size() % 2 == 1)
          {
            const usize halfElements = static_cast<usize>(std::floor(values.size() / 2.0f));
            m_MedianArray->setValue(k, values[halfElements]);
          }
          else
          {
            const usize idxLow = (values.size() / 2) - 1;
            const usize idxHigh = values.size() / 2;
            m_MedianArray->setValue(k, (values[idxLow] + values[idxHigh]) * 0.5f);
          }
        }
      }
      if(m_FindNumUniqueValues)
      {
        m_NumUniqueValuesArray->setValue(k, static_cast<int32>(perFeatureUniqueValues[k].size()));
      }
    }

    return {};
  }

private:
  bool m_FindMedian;
  bool m_FindNumUniqueValues;
  Float32ArrayType* m_MedianArray;
  Int32ArrayType* m_NumUniqueValuesArray;
  const std::vector<usize>& m_FeatureIdToCompactIndex;
  const std::unique_ptr<mask_compare_utilities::MaskCompare>& m_Mask;
  const Int32AbstractDataStoreType& m_FeatureIds;
  const AbstractDataStore<T>& m_Source;
  int32 m_Start;
  usize m_NumFeatures;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Computes requested statistics for one resident ungrouped container.
 * @tparam ContainerType Resident input container type.
 * @tparam T Input value type.
 * @param data Input values.
 * @param arrays Created output arrays in filter-defined order.
 * @param inputValues Selected statistics and output paths.
 * @return Error for an incompatible checked output array.
 * @throws std::invalid_argument If a median, mode, or unique-count output has an incompatible type.
 */
template <class ContainerType, typename T>
Result<> FindStatisticsImpl(const ContainerType& data, std::vector<IArray*>& arrays, const ComputeArrayStatisticsInputValues* inputValues)
{
  if(inputValues->find_length)
  {
    auto* array0Ptr = dynamic_cast<DataArray<uint64>*>(arrays[0]);
    if(array0Ptr == nullptr)
    {
      return MakeErrorResult(-563501, fmt::format("Could not cast 'Length' array at path '{}' to UInt64Array.", inputValues->length_array_name.toString()));
    }
    const auto val = static_cast<uint64>(data.size());
    array0Ptr->initializeTuple(0, val);
  }

  // One min/max reduction serves either requested extrema output.
  if(inputValues->find_min || inputValues->find_max)
  {
    const std::pair<T, T> minMaxValues = statistics_calculations::FindMinMax(data);
    if(inputValues->find_min)
    {
      auto* array1Ptr = dynamic_cast<DataArray<T>*>(arrays[1]);
      if(array1Ptr == nullptr)
      {
        return MakeErrorResult(-563501, fmt::format("Could not cast 'Minimum' array at path '{}' to the expected type.", inputValues->minimum_array_name.toString()));
      }
      array1Ptr->initializeTuple(0, minMaxValues.first);
    }
    if(inputValues->find_max)
    {
      auto* array2Ptr = dynamic_cast<DataArray<T>*>(arrays[2]);
      if(array2Ptr == nullptr)
      {
        return MakeErrorResult(-563501, fmt::format("Could not cast 'Maximum' array at path '{}' to the expected type.", inputValues->maximum_array_name.toString()));
      }
      array2Ptr->initializeTuple(0, minMaxValues.second);
    }
  }

  // Mean and standard deviation both need the sum.
  if(inputValues->find_summation || inputValues->find_mean || inputValues->find_std_deviation)
  {
    const std::pair<float32, float32> sumMeanValues = statistics_calculations::FindSumMean(data);
    if(inputValues->find_summation)
    {
      auto* array6Ptr = dynamic_cast<Float32ArrayType*>(arrays[7]);
      if(array6Ptr == nullptr)
      {
        return MakeErrorResult(-563501, fmt::format("Could not cast 'Summation' array at path '{}' to Float32Array.", inputValues->summation_array_name.toString()));
      }
      array6Ptr->initializeTuple(0, sumMeanValues.first);
    }
    if(inputValues->find_mean)
    {
      auto* array3Ptr = dynamic_cast<Float32ArrayType*>(arrays[3]);
      if(array3Ptr == nullptr)
      {
        return MakeErrorResult(-563501, fmt::format("Could not cast 'Mean' array at path '{}' to Float32Array.", inputValues->mean_array_name.toString()));
      }
      array3Ptr->initializeTuple(0, sumMeanValues.second);
    }
    if(inputValues->find_std_deviation)
    {
      auto* array5Ptr = dynamic_cast<Float32ArrayType*>(arrays[6]);
      if(array5Ptr == nullptr)
      {
        return MakeErrorResult(-563501, fmt::format("Could not cast 'Standard Deviation' array at path '{}' to Float32Array.", inputValues->std_deviation_array_name.toString()));
      }
      const float32 val = statistics_calculations::FindStdDeviation(data, sumMeanValues);
      array5Ptr->initializeTuple(0, val);
    }
  }

  if(inputValues->find_median)
  {
    auto* array4Ptr = dynamic_cast<Float32ArrayType*>(arrays[4]);
    if(array4Ptr == nullptr)
    {
      throw std::invalid_argument(fmt::format("Could not cast 'Median' array at path '{}' to Float32Array.", inputValues->median_array_name.toString()));
    }
    const float32 val = statistics_calculations::FindMedian(data);
    array4Ptr->initializeTuple(0, val);
  }

  if constexpr(!std::is_same_v<T, bool>)
  {
    if(inputValues->find_mode)
    {
      auto* array5Ptr = dynamic_cast<NeighborList<T>*>(arrays[5]);
      if(array5Ptr == nullptr)
      {
        throw std::invalid_argument(fmt::format("Could not cast 'Mode' array at path '{}' to the expected NeighborList type.", inputValues->mode_array_name.toString()));
      }
      std::vector<T> modes = statistics_calculations::FindModes(data);
      for(const auto& mode : modes)
      {
        array5Ptr->addEntry(0, mode);
      }
    }
  }

  if(inputValues->find_num_unique_values)
  {
    auto* array8Ptr = dynamic_cast<DataArray<int32>*>(arrays[8]);
    if(array8Ptr == nullptr)
    {
      throw std::invalid_argument(fmt::format("Could not cast 'Number of Unique Values' array at path '{}' to Int32Array.", inputValues->num_unique_values_name.toString()));
    }
    const auto val = static_cast<int32>(statistics_calculations::FindNumUniqueValues(data));
    array8Ptr->initializeTuple(0, val);
  }

  return {};
}

/**
 * @brief Initializes enabled statistics outputs to their empty-group sentinels.
 * @tparam T Input and extrema value type.
 * @param dataStructure Owns the created output arrays.
 * @param inputValues Selected statistics and output paths.
 * @return Error for a missing or incompatible output array.
 */
template <typename T>
Result<> InitializeArrays(DataStructure& dataStructure, const ComputeArrayStatisticsInputValues* inputValues)
{
  using InputDataArrayType = DataArray<T>;
  if(inputValues->compute_by_index)
  {
    auto* arrayPtr = dataStructure.getDataAs<BoolArrayType>(inputValues->feature_has_data_array_name);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563502, fmt::format("Could not find or cast 'Feature Has Data' array at path '{}' to BoolArray.", inputValues->feature_has_data_array_name.toString()));
    }
    arrayPtr->fill(false);
  }
  if(inputValues->find_length)
  {
    auto* arrayPtr = dataStructure.getDataAs<UInt64ArrayType>(inputValues->length_array_name);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563503, fmt::format("Could not find or cast 'Length' array at path '{}' to UInt64Array.", inputValues->length_array_name.toString()));
    }
    arrayPtr->fill(0ULL);
  }
  if(inputValues->find_min)
  {
    auto* arrayPtr = dataStructure.getDataAs<InputDataArrayType>(inputValues->minimum_array_name);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563504, fmt::format("Could not find or cast 'Minimum' array at path '{}' to the expected type.", inputValues->minimum_array_name.toString()));
    }
    arrayPtr->fill(static_cast<T>(std::numeric_limits<T>::max()));
  }
  if(inputValues->find_max)
  {
    auto* arrayPtr = dataStructure.getDataAs<InputDataArrayType>(inputValues->maximum_array_name);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563505, fmt::format("Could not find or cast 'Maximum' array at path '{}' to the expected type.", inputValues->maximum_array_name.toString()));
    }
    arrayPtr->fill(static_cast<T>(std::numeric_limits<T>::min()));
  }
  if(inputValues->find_mean)
  {
    auto* arrayPtr = dataStructure.getDataAs<Float32ArrayType>(inputValues->mean_array_name);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563506, fmt::format("Could not find or cast 'Mean' array at path '{}' to Float32Array.", inputValues->mean_array_name.toString()));
    }
    arrayPtr->fill(0.0F);
  }
  if(inputValues->find_median)
  {
    auto* arrayPtr = dataStructure.getDataAs<Float32ArrayType>(inputValues->median_array_name);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563507, fmt::format("Could not find or cast 'Median' array at path '{}' to Float32Array.", inputValues->median_array_name.toString()));
    }
    arrayPtr->fill(0.0F);
  }
  if(inputValues->find_std_deviation)
  {
    auto* arrayPtr = dataStructure.getDataAs<Float32ArrayType>(inputValues->std_deviation_array_name);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563509, fmt::format("Could not find or cast 'Standard Deviation' array at path '{}' to Float32Array.", inputValues->std_deviation_array_name.toString()));
    }
    arrayPtr->fill(0.0F);
  }
  if(inputValues->find_summation)
  {
    auto* arrayPtr = dataStructure.getDataAs<Float32ArrayType>(inputValues->summation_array_name);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563510, fmt::format("Could not find or cast 'Summation' array at path '{}' to Float32Array.", inputValues->summation_array_name.toString()));
    }
    arrayPtr->fill(0.0F);
  }
  if(inputValues->find_num_unique_values)
  {
    auto* arrayPtr = dataStructure.getDataAs<Int32ArrayType>(inputValues->num_unique_values_name);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563513, fmt::format("Could not find or cast 'Number of Unique Values' array at path '{}' to Int32Array.", inputValues->num_unique_values_name.toString()));
    }
    arrayPtr->fill(-1);
  }
  return {};
}

/**
 * @struct ComputeArrayStatisticsFunctor
 * @brief Dispatches resident ungrouped statistics by input type.
 */
struct ComputeArrayStatisticsFunctor
{
  /**
   * @brief Computes resident ungrouped statistics for one input type.
   * @tparam T Input value type.
   * @param dataStructure Owns inputs and outputs.
   * @param inputIDataArray Input array.
   * @param arrays Created output arrays in filter-defined order.
   * @param inputValues Selected statistics and output paths.
   * @return Initialization or statistics result.
   */
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const IDataArray& inputIDataArray, std::vector<IArray*>& arrays, const ComputeArrayStatisticsInputValues* inputValues)
  {
    std::unique_ptr<mask_compare_utilities::MaskCompare> maskCompare = nullptr;
    if(inputValues->use_mask)
    {
      try
      {
        maskCompare = mask_compare_utilities::InstantiateMaskCompare(dataStructure, inputValues->mask_array_path);
      } catch(const std::out_of_range& exception)
      {
        // Direct callers can bypass preflight, so the runtime check returns a usable error.
        const std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", inputValues->mask_array_path.toString());
        return MakeErrorResult(-563501, message);
      }
    }

    Result<> initializationResult = InitializeArrays<T>(dataStructure, inputValues);
    if(initializationResult.invalid())
    {
      return initializationResult;
    }

    const auto& inputArray = static_cast<const DataArray<T>&>(inputIDataArray);
    if(inputValues->use_mask)
    {
      // The resident direct path materializes accepted values for ungrouped masked statistics.
      const usize numTuples = inputArray.getNumberOfTuples();
      std::vector<T> data;
      data.reserve(numTuples);
      for(usize i = 0; i < numTuples; i++)
      {
        if(maskCompare->isTrue(i))
        {
          data.push_back(inputArray[i]);
        }
      }
      data.shrink_to_fit();
      Result<> result = FindStatisticsImpl<std::vector<T>, T>(data, arrays, inputValues);
      if(result.invalid())
      {
        return result;
      }
    }
    else
    {
      Result<> result = FindStatisticsImpl<DataArray<T>, T>(inputArray, arrays, inputValues);
      if(result.invalid())
      {
        return result;
      }
    }

    // Resident standardization reads input values directly after mean and deviation exist.
    if(inputValues->standardize_data)
    {
      const auto& mean = dataStructure.getDataRefAs<Float32ArrayType>(inputValues->mean_array_name).getDataStoreRef();
      const auto& std = dataStructure.getDataRefAs<Float32ArrayType>(inputValues->std_deviation_array_name).getDataStoreRef();
      auto& standardized = dataStructure.getDataRefAs<Float32ArrayType>(inputValues->standardized_array_name).getDataStoreRef();
      auto& data = inputArray.getDataStoreRef();

      const usize numTuples = data.getNumberOfTuples();

      for(usize i = 0; i < numTuples; i++)
      {
        if(!inputValues->use_mask || maskCompare->isTrue(i))
        {
          standardized.setValue(i, (static_cast<float32>(data[i]) - mean[0]) / std[0]);
        }
      }
    }
    return {};
  }
};

/**
 * @struct ComputeArrayStatisticsByFeatureFunctor
 * @brief Dispatches resident grouped statistics by input type and range mode.
 */
struct ComputeArrayStatisticsByFeatureFunctor
{
  /**
   * @brief Computes resident statistics for the legacy None feature range.
   * @tparam T Input value type.
   * @param dataStructure Owns inputs and outputs.
   * @param inputIDataArray Input array.
   * @param arrays Created output arrays in filter-defined order.
   * @param numFeatures Number of output features.
   * @param inputValues Selected statistics and output paths.
   * @param shouldCancel Cancellation flag.
   * @param algorithm Serializes progress and status messages.
   * @return Initialization or resident statistics result.
   *
   * When multicore support is enabled, TBB runs only when every participating store is resident.
   * Otherwise, the in-memory requirement gate runs this worker serially.
   */
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const IDataArray* inputIDataArray, std::vector<IArray*>& arrays, usize numFeatures, const ComputeArrayStatisticsInputValues* inputValues,
                      const std::atomic_bool& shouldCancel, ComputeArrayStatistics& algorithm)
  {
    std::unique_ptr<mask_compare_utilities::MaskCompare> maskCompare = nullptr;
    if(inputValues->use_mask)
    {
      try
      {
        maskCompare = mask_compare_utilities::InstantiateMaskCompare(dataStructure, inputValues->mask_array_path);
      } catch(const std::out_of_range& exception)
      {
        // Direct callers can bypass preflight, so the runtime check returns a usable error.
        const std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", inputValues->mask_array_path.toString());
        return MakeErrorResult(-563508, message);
      }
    }

    Result<> initializationResult = InitializeArrays<T>(dataStructure, inputValues);
    if(initializationResult.invalid())
    {
      return initializationResult;
    }

    const auto* inputArrayPtr = static_cast<const DataArray<T>*>(inputIDataArray);
    const auto* featureIdsPtr = dataStructure.getDataAs<Int32ArrayType>(inputValues->feature_ids_array_path);
    auto* lengthArrayPtr = dynamic_cast<DataArray<uint64>*>(arrays[0]);
    auto* minArrayPtr = dynamic_cast<DataArray<T>*>(arrays[1]);
    auto* maxArrayPtr = dynamic_cast<DataArray<T>*>(arrays[2]);
    auto* meanArrayPtr = dynamic_cast<Float32ArrayType*>(arrays[3]);
    auto* modeArrayPtr = dynamic_cast<NeighborList<T>*>(arrays[5]);
    auto* stdDevArrayPtr = dynamic_cast<Float32ArrayType*>(arrays[6]);
    auto* summationArrayPtr = dynamic_cast<Float32ArrayType*>(arrays[7]);

    auto* featureHasDataPtr = dynamic_cast<BoolArrayType*>(arrays[9]);

    IParallelAlgorithm::AlgorithmArraysType indexAlgArrays;
    indexAlgArrays.push_back(inputArrayPtr);
    indexAlgArrays.push_back(featureHasDataPtr);
    indexAlgArrays.push_back(lengthArrayPtr);
    indexAlgArrays.push_back(minArrayPtr);
    indexAlgArrays.push_back(maxArrayPtr);
    indexAlgArrays.push_back(meanArrayPtr);
    indexAlgArrays.push_back(stdDevArrayPtr);
    indexAlgArrays.push_back(summationArrayPtr);

    const auto& featureIds = featureIdsPtr->getDataStoreRef();
    auto& data = inputArrayPtr->getDataStoreRef();
    StatisticsByFeatureImpl<T> classToExecute = StatisticsByFeatureImpl<T>(inputValues->find_length, inputValues->find_min, inputValues->find_max, inputValues->find_mean, inputValues->find_mode,
                                                                           inputValues->find_std_deviation, inputValues->find_summation, maskCompare, featureIds, data, featureHasDataPtr, lengthArrayPtr,
                                                                           minArrayPtr, maxArrayPtr, meanArrayPtr, modeArrayPtr, stdDevArrayPtr, summationArrayPtr, shouldCancel, algorithm);
    // Weight tuple scans by the feature range so progress does not depend on TBB partitioning.
    const usize tuplePasses = inputValues->find_std_deviation ? 2 : 1;
    algorithm.resetProgress(numFeatures * (data.getNumberOfTuples() * tuplePasses + 1));
#ifdef SIMPLNX_ENABLE_MULTICORE
    if(CheckArraysInMemory(indexAlgArrays))
    {
      const tbb::simple_partitioner simplePartitioner;
      const usize grainSize = 500;
      tbb::blocked_range<usize> tbbRange(0, numFeatures, grainSize);
      tbb::parallel_for(tbbRange, std::move(classToExecute), simplePartitioner);
    }
    else
#endif
    {
      ParallelDataAlgorithm indexAlg;
      indexAlg.setRange(0, numFeatures);
      indexAlg.requireArraysInMemory(indexAlgArrays);
      indexAlg.execute(std::move(classToExecute));
    }

    if(inputValues->find_median || inputValues->find_num_unique_values)
    {
      algorithm.sendThreadSafeInfoMessage("Starting Median Calculation...");

      auto* medianArrayPtr = dynamic_cast<Float32ArrayType*>(arrays[4]);
      auto* numUniqueValuesArrayPtr = dynamic_cast<Int32ArrayType*>(arrays[8]);

      ParallelDataAlgorithm medianDataAlg;
      {
        // This temporary list only determines whether median work can run in parallel.
        IParallelAlgorithm::AlgorithmArraysType medianAlgArrays;
        medianAlgArrays.push_back(featureIdsPtr);
        medianAlgArrays.push_back(inputArrayPtr);
        medianAlgArrays.push_back(medianArrayPtr);
        medianAlgArrays.push_back(numUniqueValuesArrayPtr);
        medianAlgArrays.push_back(lengthArrayPtr);

        medianDataAlg.requireArraysInMemory(medianAlgArrays);
      }
      medianDataAlg.setRange(0, numFeatures);
      medianDataAlg.execute(
          MedianByFeatureImpl<T>(maskCompare, featureIds, data, inputValues->find_median, inputValues->find_num_unique_values, medianArrayPtr, numUniqueValuesArrayPtr, lengthArrayPtr, algorithm));
    }

    // Resident standardization uses the per-feature mean and deviation arrays.
    if(inputValues->standardize_data)
    {
      const auto& mean = dataStructure.getDataRefAs<Float32ArrayType>(inputValues->mean_array_name).getDataStoreRef();
      const auto& std = dataStructure.getDataRefAs<Float32ArrayType>(inputValues->std_deviation_array_name).getDataStoreRef();
      auto& standardized = dataStructure.getDataRefAs<Float32ArrayType>(inputValues->standardized_array_name).getDataStoreRef();

      const usize numTuples = data.getNumberOfTuples();
      for(usize i = 0; i < numTuples; i++)
      {
        const int32 featureId = featureIds.at(i);
        if((!inputValues->use_mask || maskCompare->isTrue(i)) && featureId >= 0 && static_cast<usize>(featureId) < numFeatures)
        {
          standardized.setValue(i, (static_cast<float32>(data[i]) - mean[static_cast<usize>(featureId)]) / std[static_cast<usize>(featureId)]);
        }
      }
    }
    return {};
  }

  /**
   * @brief Computes resident statistics for a configured feature range.
   * @tparam T Input value type.
   * @param dataStructure Owns inputs and outputs.
   * @param inputIDataArray Input array.
   * @param arrays Created output arrays in filter-defined order.
   * @param range Inclusive feature identifier range.
   * @param inputValues Selected statistics and output paths.
   * @param shouldCancel Cancellation flag.
   * @return Initialization, bulk-I/O, or cancellation result.
   *
   * One feature-ID map creates compact output indices. Statistics, exact median,
   * unique values, and standardization reuse this feature-scale lookup.
   */
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const IDataArray* inputIDataArray, std::vector<IArray*>& arrays, const std::pair<int32, int32>& range,
                      const ComputeArrayStatisticsInputValues* inputValues, const std::atomic_bool& shouldCancel)
  {
    std::unique_ptr<mask_compare_utilities::MaskCompare> maskCompare = nullptr;
    if(inputValues->use_mask)
    {
      try
      {
        maskCompare = mask_compare_utilities::InstantiateMaskCompare(dataStructure, inputValues->mask_array_path);
      } catch(const std::out_of_range&)
      {
        return MakeErrorResult(-563508, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", inputValues->mask_array_path.toString()));
      }
    }

    Result<> initializationResult = InitializeArrays<T>(dataStructure, inputValues);
    if(initializationResult.invalid())
    {
      return initializationResult;
    }

    const auto* featureIdsMapPtr = dataStructure.getDataAs<Int32ArrayType>(inputValues->feature_id_map_array_path);
    const auto* inputArrayPtr = static_cast<const DataArray<T>*>(inputIDataArray);
    const auto* featureIdsPtr = dataStructure.getDataAs<Int32ArrayType>(inputValues->feature_ids_array_path);
    auto* lengthArrayPtr = dynamic_cast<DataArray<uint64>*>(arrays[0]);
    auto* minArrayPtr = dynamic_cast<DataArray<T>*>(arrays[1]);
    auto* maxArrayPtr = dynamic_cast<DataArray<T>*>(arrays[2]);
    auto* meanArrayPtr = dynamic_cast<Float32ArrayType*>(arrays[3]);
    auto* modeArrayPtr = dynamic_cast<NeighborList<T>*>(arrays[5]);
    auto* stdDevArrayPtr = dynamic_cast<Float32ArrayType*>(arrays[6]);
    auto* summationArrayPtr = dynamic_cast<Float32ArrayType*>(arrays[7]);

    auto* featureHasDataPtr = dynamic_cast<BoolArrayType*>(arrays[9]);

    const auto& featureIds = featureIdsPtr->getDataStoreRef();
    const auto& featureIdsMapStore = featureIdsMapPtr->getDataStoreRef();
    const auto& data = inputArrayPtr->getDataStoreRef();

    const int32 start = range.first;
    const auto numFeatures = static_cast<usize>(static_cast<int64>(range.second) - static_cast<int64>(range.first) + 1);

    // Cache the feature map once for all feature-scale and cell-level operations.
    std::vector<int32> featureIdMapCache(numFeatures);
    Result<> mapReadResult = featureIdsMapStore.copyIntoBuffer(0, nonstd::span<int32>(featureIdMapCache.data(), numFeatures));
    if(mapReadResult.invalid())
    {
      return mapReadResult;
    }
    std::vector<usize> featureIdToCompactIndex(numFeatures);
    for(usize k = 0; k < numFeatures; k++)
    {
      featureIdToCompactIndex[static_cast<usize>(static_cast<int64>(featureIdMapCache[k]) - static_cast<int64>(start))] = k;
    }

    StatisticsByFeatureRangeImpl<T> statsImpl(inputValues->find_length, inputValues->find_min, inputValues->find_max, inputValues->find_mean, inputValues->find_mode, inputValues->find_std_deviation,
                                              inputValues->find_summation, featureIdToCompactIndex, maskCompare, featureIds, data, start, numFeatures, featureHasDataPtr, lengthArrayPtr, minArrayPtr,
                                              maxArrayPtr, meanArrayPtr, modeArrayPtr, stdDevArrayPtr, summationArrayPtr, shouldCancel);
    Result<> statsResult = statsImpl();
    if(statsResult.invalid())
    {
      return statsResult;
    }

    if(inputValues->find_median || inputValues->find_num_unique_values)
    {
      auto* medianArrayPtr = dynamic_cast<Float32ArrayType*>(arrays[4]);
      auto* numUniqueValuesArrayPtr = dynamic_cast<Int32ArrayType*>(arrays[8]);

      MedianByFeatureRangeImpl<T> medianImpl(featureIdToCompactIndex, maskCompare, featureIds, data, start, numFeatures, inputValues->find_median, inputValues->find_num_unique_values, medianArrayPtr,
                                             numUniqueValuesArrayPtr, shouldCancel);
      Result<> medianResult = medianImpl();
      if(medianResult.invalid())
      {
        return medianResult;
      }
    }

    // Standardization uses the compact feature layout.
    if(inputValues->standardize_data)
    {
      const auto& mean = dataStructure.getDataRefAs<Float32ArrayType>(inputValues->mean_array_name).getDataStoreRef();
      const auto& stdDevStore = dataStructure.getDataRefAs<Float32ArrayType>(inputValues->std_deviation_array_name).getDataStoreRef();
      auto& standardized = dataStructure.getDataRefAs<Float32ArrayType>(inputValues->standardized_array_name).getDataStoreRef();

      // Feature-scale means and deviations stay resident. Page read-modify-write
      // preserves standardized values for tuples outside the selected range.
      std::vector<float32> meanCache(numFeatures);
      Result<> meanReadResult = mean.copyIntoBuffer(0, nonstd::span<float32>(meanCache.data(), numFeatures));
      if(meanReadResult.invalid())
      {
        return meanReadResult;
      }
      std::vector<float32> stdDevCache(numFeatures);
      Result<> stdDevReadResult = stdDevStore.copyIntoBuffer(0, nonstd::span<float32>(stdDevCache.data(), numFeatures));
      if(stdDevReadResult.invalid())
      {
        return stdDevReadResult;
      }

      const usize numTuples = data.getNumberOfTuples();
      auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
      auto sourceBuf = std::make_unique<T[]>(k_ChunkTuples);
      auto standardizedBuf = std::make_unique<float32[]>(k_ChunkTuples);

      for(usize offset = 0; offset < numTuples; offset += k_ChunkTuples)
      {
        if(shouldCancel)
        {
          return {};
        }
        const usize chunkCount = std::min(k_ChunkTuples, numTuples - offset);

        Result<> featureIdResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), chunkCount));
        if(featureIdResult.invalid())
        {
          return featureIdResult;
        }
        Result<> sourceResult = data.copyIntoBuffer(offset, nonstd::span<T>(sourceBuf.get(), chunkCount));
        if(sourceResult.invalid())
        {
          return sourceResult;
        }
        Result<> standardizedReadResult = standardized.copyIntoBuffer(offset, nonstd::span<float32>(standardizedBuf.get(), chunkCount));
        if(standardizedReadResult.invalid())
        {
          return standardizedReadResult;
        }

        for(usize idx = 0; idx < chunkCount; idx++)
        {
          const int32 featureId = featureIdBuf[idx];
          if((maskCompare != nullptr && !maskCompare->isTrue(offset + idx)) || featureId < range.first || featureId > range.second)
          {
            continue;
          }
          const usize compactIndex = featureIdToCompactIndex[static_cast<usize>(static_cast<int64>(featureId) - static_cast<int64>(start))];
          standardizedBuf[idx] = (static_cast<float32>(sourceBuf[idx]) - meanCache[compactIndex]) / stdDevCache[compactIndex];
        }

        Result<> standardizedWriteResult = standardized.copyFromBuffer(offset, nonstd::span<const float32>(standardizedBuf.get(), chunkCount));
        if(standardizedWriteResult.invalid())
        {
          return standardizedWriteResult;
        }
      }
    }
    return {};
  }
};

/**
 * @struct StatisticsGroupLayout
 * @brief Maps FeatureIds into the compact output tuple range chosen by the filter.
 * IDs outside a custom range return no group and are skipped without allocating
 * a sparse array up to the largest possible ID.
 */
struct StatisticsGroupLayout
{
  bool compute_by_index = false;      // True when outputs group tuples by FeatureId.
  bool create_feature_id_map = false; // True when the filter creates compact-to-source mapping output.
  int32 first_feature_id = 0;         // Inclusive source FeatureId lower bound.
  int32 last_feature_id = 0;          // Inclusive source FeatureId upper bound.
  usize group_count = 1;              // Number of compact output tuples.

  /**
   * @brief Returns a compact output index for one FeatureId.
   * @param featureId Source FeatureId.
   * @return Compact output index, or no value when featureId is excluded.
   */
  std::optional<usize> groupIndex(int32 featureId) const
  {
    if(!compute_by_index)
    {
      return usize{0};
    }
    if(group_count == 0 || featureId < first_feature_id || featureId > last_feature_id)
    {
      return std::nullopt;
    }
    return static_cast<usize>(static_cast<int64>(featureId) - static_cast<int64>(first_feature_id));
  }
};

/**
 * @struct ObservedFeatureBounds
 * @brief Stores FeatureId bounds from one bounded scan.
 */
struct ObservedFeatureBounds
{
  bool has_values = false; // True after at least one FeatureId is observed.
  int32 minimum = 0;       // Lowest observed FeatureId.
  int32 maximum = 0;       // Highest observed FeatureId.
};

/**
 * @brief Finds the observed FeatureId interval with fixed-size bulk reads.
 * @param featureIds Feature identifier store.
 * @param shouldCancel Cancellation flag.
 * @return Partial bounds on cancellation, a bulk-read error, or complete bounds.
 *
 * Fixed pages avoid a FeatureIds-sized resident buffer.
 */
Result<ObservedFeatureBounds> DiscoverFeatureBounds(const Int32AbstractDataStoreType& featureIds, const std::atomic_bool& shouldCancel)
{
  ObservedFeatureBounds bounds;
  auto featureBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  const usize tupleCount = featureIds.getNumberOfTuples();
  for(usize offset = 0; offset < tupleCount; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {bounds};
    }
    const usize count = std::min(k_ChunkTuples, tupleCount - offset);
    Result<> readResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureBuffer.get(), count));
    if(readResult.invalid())
    {
      return ConvertInvalidResult<ObservedFeatureBounds>(std::move(readResult));
    }
    for(usize index = 0; index < count; ++index)
    {
      const int32 featureId = featureBuffer[index];
      if(!bounds.has_values)
      {
        bounds.has_values = true;
        bounds.minimum = featureId;
        bounds.maximum = featureId;
      }
      else
      {
        bounds.minimum = std::min(bounds.minimum, featureId);
        bounds.maximum = std::max(bounds.maximum, featureId);
      }
    }
  }
  return {bounds};
}

/**
 * @brief Combines observed IDs and range controls into a dense output layout.
 * @param inputValues Filter grouping options.
 * @param observedBounds FeatureId bounds from the input.
 * @return Error for an invalid or unrepresentable range, or the output layout.
 */
Result<StatisticsGroupLayout> ResolveGroupLayout(const ComputeArrayStatisticsInputValues& inputValues, const ObservedFeatureBounds& observedBounds)
{
  StatisticsGroupLayout layout;
  layout.compute_by_index = inputValues.compute_by_index;
  if(!layout.compute_by_index)
  {
    return {layout};
  }

  const auto selection = static_cast<ComputeArrayStatistics::FeatureIdRangeControls>(inputValues.range_type);
  layout.create_feature_id_map = selection != ComputeArrayStatistics::FeatureIdRangeControls::None;
  if(!observedBounds.has_values)
  {
    if(selection == ComputeArrayStatistics::FeatureIdRangeControls::PaddedCustomRange && inputValues.range.at(1) != -1)
    {
      layout.first_feature_id = inputValues.range.at(0);
      layout.last_feature_id = inputValues.range.at(1);
    }
    else
    {
      layout.group_count = 0;
      return {layout};
    }
  }
  else
  {
    layout.first_feature_id = observedBounds.minimum;
    layout.last_feature_id = observedBounds.maximum;
    switch(selection)
    {
    case ComputeArrayStatistics::FeatureIdRangeControls::None:
      layout.first_feature_id = 0;
      if(layout.last_feature_id < 0)
      {
        layout.group_count = 0;
        return {layout};
      }
      break;
    case ComputeArrayStatistics::FeatureIdRangeControls::IgnoreZero:
      layout.first_feature_id = 1;
      break;
    case ComputeArrayStatistics::FeatureIdRangeControls::ShrinkToFit:
      break;
    case ComputeArrayStatistics::FeatureIdRangeControls::CustomRange:
      layout.first_feature_id = std::max(layout.first_feature_id, inputValues.range.at(0));
      if(inputValues.range.at(1) != -1)
      {
        layout.last_feature_id = std::min(layout.last_feature_id, inputValues.range.at(1));
      }
      break;
    case ComputeArrayStatistics::FeatureIdRangeControls::PaddedCustomRange:
      layout.first_feature_id = inputValues.range.at(0);
      if(inputValues.range.at(1) != -1)
      {
        layout.last_feature_id = inputValues.range.at(1);
      }
      break;
    default:
      return MakeErrorResult<StatisticsGroupLayout>(
          -506670, fmt::format("ComputeArrayStatistics: unknown feature ID range control {} for observed FeatureIds [{}, {}].", inputValues.range_type, observedBounds.minimum, observedBounds.maximum));
    }
  }

  if(layout.first_feature_id > layout.last_feature_id)
  {
    return MakeErrorResult<StatisticsGroupLayout>(
        -506671, fmt::format("ComputeArrayStatistics: range minimum ({}) must be less than or equal to range maximum ({}).", layout.first_feature_id, layout.last_feature_id));
  }
  const uint64 groupCount = static_cast<uint64>(static_cast<int64>(layout.last_feature_id) - static_cast<int64>(layout.first_feature_id)) + 1ULL;
  if(groupCount > static_cast<uint64>(std::numeric_limits<int32>::max()))
  {
    return MakeErrorResult<StatisticsGroupLayout>(
        -57300, fmt::format("ComputeArrayStatistics: the requested FeatureId range [{}, {}] contains {} outputs, which exceeds the supported int32 group index range.", layout.first_feature_id,
                            layout.last_feature_id, groupCount));
  }
  if(groupCount > static_cast<uint64>(std::numeric_limits<usize>::max()))
  {
    return MakeErrorResult<StatisticsGroupLayout>(
        -57301, fmt::format("ComputeArrayStatistics: the requested FeatureId range [{}, {}] cannot be represented by the platform size type.", layout.first_feature_id, layout.last_feature_id));
  }
  layout.group_count = static_cast<usize>(groupCount);
  return {layout};
}

template <typename T>
constexpr uint64 k_StatisticsRecordSize = sizeof(int32) + sizeof(T) + sizeof(uint64); // Group, value, and tuple identifier bytes.

/**
 * @brief Serializes one exact-order statistics record.
 * @tparam T Input value type.
 * @param bytes Destination record bytes.
 * @param groupId Compact output group.
 * @param value Input value.
 * @param originalTupleIndex Source tuple identifier.
 */
template <typename T>
void EncodeStatisticsRecord(nonstd::span<std::byte> bytes, int32 groupId, T value, uint64 originalTupleIndex)
{
  std::memcpy(bytes.data(), &groupId, sizeof(groupId));
  std::memcpy(bytes.data() + sizeof(groupId), &value, sizeof(value));
  std::memcpy(bytes.data() + sizeof(groupId) + sizeof(value), &originalTupleIndex, sizeof(originalTupleIndex));
}

/**
 * @brief Deserializes one exact-order statistics record.
 * @tparam T Input value type.
 * @param bytes Source record bytes.
 * @param groupId Receives the compact output group.
 * @param value Receives the input value.
 * @param originalTupleIndex Receives the source tuple identifier.
 */
template <typename T>
void DecodeStatisticsRecord(nonstd::span<const std::byte> bytes, int32& groupId, T& value, uint64& originalTupleIndex)
{
  std::memcpy(&groupId, bytes.data(), sizeof(groupId));
  std::memcpy(&value, bytes.data() + sizeof(groupId), sizeof(value));
  std::memcpy(&originalTupleIndex, bytes.data() + sizeof(groupId) + sizeof(value), sizeof(originalTupleIndex));
}

/**
 * @brief Orders scratch records by compact group, value, then input position.
 * @tparam T Input value type.
 * @param left First serialized statistics record.
 * @param right Second serialized statistics record.
 * @return Negative, zero, or positive lexical comparison result.
 *
 * The tuple identifier makes equal-value ordering deterministic without changing run counts.
 */
template <typename T>
int32 CompareStatisticsRecords(nonstd::span<const std::byte> left, nonstd::span<const std::byte> right)
{
  int32 leftGroup = 0;
  int32 rightGroup = 0;
  T leftValue{};
  T rightValue{};
  uint64 leftIndex = 0;
  uint64 rightIndex = 0;
  DecodeStatisticsRecord(left, leftGroup, leftValue, leftIndex);
  DecodeStatisticsRecord(right, rightGroup, rightValue, rightIndex);
  if(leftGroup != rightGroup)
  {
    return leftGroup < rightGroup ? -1 : 1;
  }
  if(leftValue < rightValue)
  {
    return -1;
  }
  if(rightValue < leftValue)
  {
    return 1;
  }
  if(leftIndex == rightIndex)
  {
    return 0;
  }
  return leftIndex < rightIndex ? -1 : 1;
}

/**
 * @brief Tests numeric equality with only operator<.
 * @tparam T Input value type.
 * @param left First value.
 * @param right Second value.
 * @return True when neither value is less than the other.
 */
template <typename T>
bool EquivalentStatisticsValues(const T& left, const T& right)
{
  return !(left < right) && !(right < left);
}

/**
 * @brief Streams sorted records in bounded pages and reports each equal-value run.
 * @tparam T Input value type.
 * @tparam RunFunction Callable that accepts group, value, count, and position.
 * @param externalSort Finished statistics sorter.
 * @param groupCount Number of compact output groups.
 * @param shouldCancel Cancellation flag.
 * @param runFunction Receives each equal-value run.
 * @return Sort I/O, callback, or cancellation result.
 */
template <typename T, typename RunFunction>
Result<> ScanSortedStatisticsRuns(const IExternalSort& externalSort, usize groupCount, const std::atomic_bool& shouldCancel, RunFunction&& runFunction)
{
  std::vector<std::byte> bytes(k_ChunkTuples * static_cast<usize>(k_StatisticsRecordSize<T>));
  std::optional<int32> currentGroup;
  std::optional<T> currentValue;
  uint64 currentCount = 0;
  uint64 groupPosition = 0;

  const auto flushRun = [&]() -> Result<> {
    if(!currentGroup.has_value())
    {
      return {};
    }
    if(*currentGroup < 0 || static_cast<usize>(*currentGroup) >= groupCount)
    {
      return MakeErrorResult(-57302, fmt::format("ComputeArrayStatistics: external sort returned invalid group index {} for {} output groups.", *currentGroup, groupCount));
    }
    Result<> result = runFunction(static_cast<usize>(*currentGroup), *currentValue, currentCount, groupPosition);
    if(result.invalid())
    {
      return result;
    }
    groupPosition += currentCount;
    return {};
  };

  const uint64 totalRecords = externalSort.recordCount();
  for(uint64 offset = 0; offset < totalRecords; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }
    const uint64 count = std::min<uint64>(k_ChunkTuples, totalRecords - offset);
    Result<uint64> readResult = externalSort.read(offset, count, nonstd::span<std::byte>(bytes.data(), static_cast<usize>(count * k_StatisticsRecordSize<T>)), shouldCancel);
    if(readResult.invalid())
    {
      return ConvertResult(std::move(readResult));
    }
    if(readResult.value() != count)
    {
      return MakeErrorResult(-57303, fmt::format("ComputeArrayStatistics: external sort short read at record {}: requested {} records but received {}.", offset, count, readResult.value()));
    }
    for(uint64 index = 0; index < count; ++index)
    {
      int32 group = 0;
      T value{};
      uint64 originalIndex = 0;
      const usize byteOffset = static_cast<usize>(index * k_StatisticsRecordSize<T>);
      DecodeStatisticsRecord(nonstd::span<const std::byte>(bytes.data() + byteOffset, static_cast<usize>(k_StatisticsRecordSize<T>)), group, value, originalIndex);
      if(currentGroup.has_value() && *currentGroup == group && EquivalentStatisticsValues(*currentValue, value))
      {
        if(currentCount == std::numeric_limits<uint64>::max())
        {
          return MakeErrorResult(-57304, fmt::format("ComputeArrayStatistics: occurrence count for group {} and a sorted value exceeds uint64.", group));
        }
        ++currentCount;
        continue;
      }
      Result<> flushResult = flushRun();
      if(flushResult.invalid())
      {
        return flushResult;
      }
      if(!currentGroup.has_value() || *currentGroup != group)
      {
        groupPosition = 0;
      }
      currentGroup = group;
      currentValue = value;
      currentCount = 1;
    }
  }
  return flushRun();
}

/**
 * @brief Validates and bulk-writes one feature-scale scalar output.
 * @tparam T Output value type.
 * @param outputArray Destination scalar array.
 * @param values Source values.
 * @param count Number of values.
 * @param path Output path for diagnostics.
 * @return Validation or bulk-write result.
 *
 * The contiguous transfer avoids one disk access per output value.
 */
template <typename T>
Result<> WriteStatisticsOutput(DataArray<T>* outputArray, const T* values, usize count, const DataPath& path)
{
  if(outputArray == nullptr)
  {
    return MakeErrorResult(-57305, fmt::format("ComputeArrayStatistics: required output array '{}' is missing or has the wrong type.", path.toString()));
  }
  if(outputArray->getNumberOfTuples() != count || outputArray->getNumberOfComponents() != 1)
  {
    return MakeErrorResult(-57306, fmt::format("ComputeArrayStatistics: output array '{}' has {} tuples and {} components; expected {} scalar tuples.", path.toString(),
                                               outputArray->getNumberOfTuples(), outputArray->getNumberOfComponents(), count));
  }
  return outputArray->getDataStoreRef().copyFromBuffer(0, nonstd::span<const T>(values, count));
}

/**
 * @brief Computes bounded statistics for one runtime-selected value and mask type.
 * @tparam T Input and extrema value type.
 * @tparam MaskT Mask value type.
 * @param dataStructure Owns input and output arrays.
 * @param inputDataArray Input values.
 * @param featureIdsDataArray Optional FeatureIds array.
 * @param maskDataArray Optional mask array.
 * @param layout Resolved compact output layout.
 * @param inputValues Selected statistics and output paths.
 * @param shouldCancel Cancellation flag.
 * @return Validation, bulk-I/O, sort, reduction, or cancellation result.
 *
 * Feature-scale accumulators remain resident while large cell data uses fixed pages.
 * Without an external sorter, repeated scans trade speed for bounded memory.
 * The fallback keeps median, mode, and unique-count results exact.
 */
template <typename T, typename MaskT>
Result<> GenerateScanlineStatistics(DataStructure& dataStructure, const IDataArray& inputDataArray, const IDataArray* featureIdsDataArray, const IDataArray* maskDataArray,
                                    const StatisticsGroupLayout& layout, const ComputeArrayStatisticsInputValues& inputValues, const std::atomic_bool& shouldCancel)
{
  const auto& inputStore = inputDataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
  const auto* featureIdsStorePtr = featureIdsDataArray == nullptr ? nullptr : &featureIdsDataArray->template getIDataStoreRefAs<AbstractDataStore<int32>>();
  const auto* maskStorePtr = maskDataArray == nullptr ? nullptr : &maskDataArray->template getIDataStoreRefAs<AbstractDataStore<MaskT>>();
  const usize tupleCount = inputStore.getNumberOfTuples();
  if((featureIdsStorePtr != nullptr && featureIdsStorePtr->getNumberOfTuples() != tupleCount) || (maskStorePtr != nullptr && maskStorePtr->getNumberOfTuples() != tupleCount))
  {
    return MakeErrorResult(-57307, fmt::format("ComputeArrayStatistics: input '{}' has {} tuples, FeatureIds has {}, and the mask has {}; all enabled cell arrays must have identical tuple counts.",
                                               inputDataArray.getName(), tupleCount, featureIdsStorePtr == nullptr ? 0 : featureIdsStorePtr->getNumberOfTuples(),
                                               maskStorePtr == nullptr ? 0 : maskStorePtr->getNumberOfTuples()));
  }
  if(layout.compute_by_index != (featureIdsStorePtr != nullptr))
  {
    return MakeErrorResult(-57308, "ComputeArrayStatistics: the resolved grouping layout and FeatureIds input are inconsistent.");
  }

  const usize groupCount = layout.group_count;
  auto* featureHasDataArrayPtr = inputValues.compute_by_index ? dataStructure.getDataAs<BoolArrayType>(inputValues.feature_has_data_array_name) : nullptr;
  auto* lengthArrayPtr = inputValues.find_length ? dataStructure.getDataAs<UInt64ArrayType>(inputValues.length_array_name) : nullptr;
  auto* minimumArrayPtr = inputValues.find_min ? dataStructure.getDataAs<DataArray<T>>(inputValues.minimum_array_name) : nullptr;
  auto* maximumArrayPtr = inputValues.find_max ? dataStructure.getDataAs<DataArray<T>>(inputValues.maximum_array_name) : nullptr;
  auto* meanArrayPtr = inputValues.find_mean ? dataStructure.getDataAs<Float32ArrayType>(inputValues.mean_array_name) : nullptr;
  auto* medianArrayPtr = inputValues.find_median ? dataStructure.getDataAs<Float32ArrayType>(inputValues.median_array_name) : nullptr;
  auto* stdDeviationArrayPtr = inputValues.find_std_deviation ? dataStructure.getDataAs<Float32ArrayType>(inputValues.std_deviation_array_name) : nullptr;
  auto* summationArrayPtr = inputValues.find_summation ? dataStructure.getDataAs<Float32ArrayType>(inputValues.summation_array_name) : nullptr;
  auto* uniqueValuesArrayPtr = inputValues.find_num_unique_values ? dataStructure.getDataAs<Int32ArrayType>(inputValues.num_unique_values_name) : nullptr;
  auto* standardizedArrayPtr = inputValues.standardize_data ? dataStructure.getDataAs<Float32ArrayType>(inputValues.standardized_array_name) : nullptr;
  if(inputValues.compute_by_index && featureHasDataArrayPtr == nullptr)
  {
    return MakeErrorResult(-57305, fmt::format("ComputeArrayStatistics: required FeatureHasData output '{}' is missing or has the wrong type.", inputValues.feature_has_data_array_name.toString()));
  }
  if(inputValues.find_mode)
  {
    if constexpr(std::is_same_v<T, bool>)
    {
      return MakeErrorResult(-57305, "ComputeArrayStatistics: Boolean input does not support Mode output.");
    }
    else if(dataStructure.getDataAs<NeighborList<T>>(inputValues.mode_array_name) == nullptr)
    {
      return MakeErrorResult(-57305, fmt::format("ComputeArrayStatistics: required Mode output '{}' is missing or has the wrong NeighborList type.", inputValues.mode_array_name.toString()));
    }
  }
  if(inputValues.standardize_data && (standardizedArrayPtr == nullptr || standardizedArrayPtr->getNumberOfTuples() != tupleCount))
  {
    return MakeErrorResult(-57306, fmt::format("ComputeArrayStatistics: standardized output '{}' is missing, has the wrong type, or does not contain {} tuples.",
                                               inputValues.standardized_array_name.toString(), tupleCount));
  }

  std::vector<uint64> lengths(groupCount, 0);
  auto minimums = std::make_unique<T[]>(groupCount);
  auto maximums = std::make_unique<T[]>(groupCount);
  // Boolean true counts must remain exact beyond float32's consecutive-integer range.
  using GroupSumType = std::conditional_t<std::is_same_v<T, bool>, uint64, float32>;
  using MeanType = std::conditional_t<std::is_same_v<T, bool>, float64, float32>;
  std::vector<GroupSumType> sums(groupCount, 0);
  std::vector<float32> means(groupCount, 0.0F);
  std::vector<float32> medians(groupCount, 0.0F);
  std::vector<float32> standardDeviations(groupCount, 0.0F);
  std::vector<int32> uniqueCounts(groupCount, 0);
  std::vector<uint64> maximumModeCounts(inputValues.find_mode ? groupCount : 0, 0);
  const bool legacyNoneRange = layout.compute_by_index && inputValues.range_type == to_underlying(ComputeArrayStatistics::FeatureIdRangeControls::None);
  for(usize group = 0; group < groupCount; ++group)
  {
    minimums[group] = std::numeric_limits<T>::max();
    if constexpr(std::is_floating_point_v<T>)
    {
      maximums[group] = legacyNoneRange ? std::numeric_limits<T>::min() : std::numeric_limits<T>::lowest();
    }
    else
    {
      maximums[group] = std::numeric_limits<T>::min();
    }
  }

  using GlobalSumType = std::conditional_t<std::is_integral_v<T>, std::conditional_t<std::is_signed_v<T>, int64, uint64>, float64>;
  GlobalSumType globalSum = 0;

  const bool needsSortedValues = inputValues.find_median || inputValues.find_mode || inputValues.find_num_unique_values;
  std::unique_ptr<IExternalSort> externalSort;
  std::vector<std::byte> recordBytes;
  if(needsSortedValues && data_store_utilities::GetIOCollection().hasExternalSortCapability())
  {
    ExternalSortConfig config;
    config.record_size = k_StatisticsRecordSize<T>;
    config.max_records_per_batch = k_ChunkTuples;
    config.compare = CompareStatisticsRecords<T>;
    Result<std::unique_ptr<IExternalSort>> createResult = data_store_utilities::GetIOCollection().createExternalSort(config);
    if(createResult.invalid())
    {
      return ConvertResult(std::move(createResult));
    }
    externalSort = std::move(createResult.value());
    recordBytes.resize(k_ChunkTuples * static_cast<usize>(k_StatisticsRecordSize<T>));
  }

  auto valueBuffer = std::make_unique<T[]>(k_ChunkTuples);
  auto featureBuffer = featureIdsStorePtr == nullptr ? nullptr : std::make_unique<int32[]>(k_ChunkTuples);
  auto maskBuffer = maskStorePtr == nullptr ? nullptr : std::make_unique<MaskT[]>(k_ChunkTuples);
  const auto readChunk = [&](usize offset, usize count) -> Result<> {
    Result<> result = inputStore.copyIntoBuffer(offset, nonstd::span<T>(valueBuffer.get(), count));
    if(result.invalid())
    {
      for(auto& error : result.errors())
      {
        error.message =
            fmt::format("ComputeArrayStatistics: bulk read failed for array '{}' over tuple range [{}, {}): {}", inputValues.selected_array_path.toString(), offset, offset + count, error.message);
      }
      return result;
    }
    if(featureIdsStorePtr != nullptr)
    {
      result = featureIdsStorePtr->copyIntoBuffer(offset, nonstd::span<int32>(featureBuffer.get(), count));
      if(result.invalid())
      {
        for(auto& error : result.errors())
        {
          error.message =
              fmt::format("ComputeArrayStatistics: bulk read failed for array '{}' over tuple range [{}, {}): {}", inputValues.feature_ids_array_path.toString(), offset, offset + count, error.message);
        }
        return result;
      }
    }
    if(maskStorePtr != nullptr)
    {
      result = maskStorePtr->copyIntoBuffer(offset, nonstd::span<MaskT>(maskBuffer.get(), count));
      if(result.invalid())
      {
        for(auto& error : result.errors())
        {
          error.message =
              fmt::format("ComputeArrayStatistics: bulk read failed for array '{}' over tuple range [{}, {}): {}", inputValues.mask_array_path.toString(), offset, offset + count, error.message);
        }
      }
    }
    return result;
  };

  // One page scan collects count, extrema, sums, and optional exact-order records.
  for(usize offset = 0; offset < tupleCount; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkTuples, tupleCount - offset);
    Result<> readResult = readChunk(offset, count);
    if(readResult.invalid())
    {
      return readResult;
    }
    if(shouldCancel)
    {
      return {};
    }
    uint64 recordCount = 0;
    for(usize index = 0; index < count; ++index)
    {
      if(maskStorePtr != nullptr && !static_cast<bool>(maskBuffer[index]))
      {
        continue;
      }
      const std::optional<usize> group = layout.groupIndex(featureIdsStorePtr == nullptr ? 0 : featureBuffer[index]);
      if(!group.has_value())
      {
        continue;
      }
      const T value = valueBuffer[index];
      const usize groupIndex = *group;
      if(lengths[groupIndex] == std::numeric_limits<uint64>::max())
      {
        return MakeErrorResult(-57310, fmt::format("ComputeArrayStatistics: selected-value count for output group {} exceeds uint64.", groupIndex));
      }
      ++lengths[groupIndex];
      if(value < minimums[groupIndex])
      {
        minimums[groupIndex] = value;
      }
      if(value > maximums[groupIndex])
      {
        maximums[groupIndex] = value;
      }
      if(layout.compute_by_index)
      {
        sums[groupIndex] = sums[groupIndex] + static_cast<GroupSumType>(value);
      }
      else
      {
        globalSum += static_cast<GlobalSumType>(value);
      }
      if(externalSort != nullptr)
      {
        EncodeStatisticsRecord<T>(nonstd::span<std::byte>(recordBytes.data() + static_cast<usize>(recordCount * k_StatisticsRecordSize<T>), static_cast<usize>(k_StatisticsRecordSize<T>)),
                                  static_cast<int32>(groupIndex), value, static_cast<uint64>(offset + index));
        ++recordCount;
      }
    }
    if(externalSort != nullptr && recordCount > 0)
    {
      Result<> appendResult = externalSort->append(recordCount, nonstd::span<const std::byte>(recordBytes.data(), static_cast<usize>(recordCount * k_StatisticsRecordSize<T>)), shouldCancel, {});
      if(appendResult.invalid())
      {
        return appendResult;
      }
    }
  }
  if(externalSort != nullptr)
  {
    if(shouldCancel)
    {
      return {};
    }
    Result<> finishResult = externalSort->finish(shouldCancel, {});
    if(finishResult.invalid() || shouldCancel)
    {
      return finishResult;
    }
  }

  if(!layout.compute_by_index && groupCount == 1)
  {
    sums[0] = static_cast<GroupSumType>(globalSum);
    if(lengths[0] == 0)
    {
      minimums[0] = T{};
      maximums[0] = T{};
    }
  }
  for(usize group = 0; group < groupCount; ++group)
  {
    if(shouldCancel)
    {
      return {};
    }
    if(lengths[group] == 0)
    {
      continue;
    }
    means[group] = static_cast<float32>(static_cast<MeanType>(sums[group]) / static_cast<MeanType>(lengths[group]));
  }

  // Variance needs completed means, so it uses a second page scan.
  if(inputValues.find_std_deviation)
  {
    // Float64 squared-difference accumulation reduces rounding loss before Float32 output.
    std::vector<float64> sumOfDifferences(groupCount, 0.0);
    for(usize offset = 0; offset < tupleCount; offset += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_ChunkTuples, tupleCount - offset);
      Result<> readResult = readChunk(offset, count);
      if(readResult.invalid())
      {
        return readResult;
      }
      if(shouldCancel)
      {
        return {};
      }
      for(usize index = 0; index < count; ++index)
      {
        if(maskStorePtr != nullptr && !static_cast<bool>(maskBuffer[index]))
        {
          continue;
        }
        const std::optional<usize> group = layout.groupIndex(featureIdsStorePtr == nullptr ? 0 : featureBuffer[index]);
        if(!group.has_value())
        {
          continue;
        }
        const float64 difference = static_cast<float64>(valueBuffer[index] - means[*group]);
        sumOfDifferences[*group] += difference * difference;
      }
    }
    for(usize group = 0; group < groupCount; ++group)
    {
      if(lengths[group] > 0)
      {
        standardDeviations[group] = static_cast<float32>(std::sqrt(sumOfDifferences[group] / static_cast<float64>(lengths[group])));
      }
    }
  }

  auto medianLower = std::make_unique<T[]>(inputValues.find_median ? groupCount : 0);
  auto medianUpper = std::make_unique<T[]>(inputValues.find_median ? groupCount : 0);
  std::vector<uint8> hasMedianLower(inputValues.find_median ? groupCount : 0, 0);
  std::vector<uint8> hasMedianUpper(inputValues.find_median ? groupCount : 0, 0);
  const auto consumeSortedRun = [&](usize group, const T& value, uint64 runCount, uint64 runPosition) -> Result<> {
    if(runCount > std::numeric_limits<uint64>::max() - runPosition)
    {
      return MakeErrorResult(-57311, fmt::format("ComputeArrayStatistics: sorted position overflow for output group {}.", group));
    }
    const uint64 runEnd = runPosition + runCount;
    if(inputValues.find_num_unique_values)
    {
      if(uniqueCounts[group] == std::numeric_limits<int32>::max())
      {
        return MakeErrorResult(-57312, fmt::format("ComputeArrayStatistics: output group {} contains more than {} unique values, which cannot be represented by the Int32 output.", group,
                                                   std::numeric_limits<int32>::max()));
      }
      ++uniqueCounts[group];
    }
    if(inputValues.find_mode)
    {
      maximumModeCounts[group] = std::max(maximumModeCounts[group], runCount);
    }
    if(inputValues.find_median && lengths[group] > 0)
    {
      const uint64 lowerPosition = (lengths[group] - 1) / 2;
      const uint64 upperPosition = lengths[group] / 2;
      if(runPosition <= lowerPosition && lowerPosition < runEnd)
      {
        medianLower[group] = value;
        hasMedianLower[group] = 1;
      }
      if(runPosition <= upperPosition && upperPosition < runEnd)
      {
        medianUpper[group] = value;
        hasMedianUpper[group] = 1;
      }
    }
    return {};
  };

  // Equal-value runs yield unique counts, mode frequencies, and exact median positions.
  if(needsSortedValues && externalSort != nullptr)
  {
    Result<> sortedResult = ScanSortedStatisticsRuns<T>(*externalSort, groupCount, shouldCancel, consumeSortedRun);
    if(sortedResult.invalid() || shouldCancel)
    {
      return sortedResult;
    }
  }
  else if(needsSortedValues)
  {
    // Provider-free scans are slower but keep exact order statistics bounded in memory.
    const auto scanGroup = [&](usize requestedGroup, auto&& valueFunction) -> Result<> {
      for(usize offset = 0; offset < tupleCount; offset += k_ChunkTuples)
      {
        if(shouldCancel)
        {
          return {};
        }
        const usize count = std::min(k_ChunkTuples, tupleCount - offset);
        Result<> readResult = readChunk(offset, count);
        if(readResult.invalid())
        {
          return readResult;
        }
        if(shouldCancel)
        {
          return {};
        }
        for(usize index = 0; index < count; ++index)
        {
          if(maskStorePtr != nullptr && !static_cast<bool>(maskBuffer[index]))
          {
            continue;
          }
          const std::optional<usize> group = layout.groupIndex(featureIdsStorePtr == nullptr ? 0 : featureBuffer[index]);
          if(group.has_value() && *group == requestedGroup)
          {
            valueFunction(valueBuffer[index]);
          }
        }
      }
      return {};
    };

    for(usize group = 0; group < groupCount; ++group)
    {
      if(shouldCancel)
      {
        return {};
      }
      std::optional<T> previous;
      uint64 groupPosition = 0;
      while(!shouldCancel)
      {
        std::optional<T> next;
        Result<> scanResult = scanGroup(group, [&](const T& value) {
          if((!previous.has_value() || *previous < value) && (!next.has_value() || value < *next))
          {
            next = value;
          }
        });
        if(scanResult.invalid() || shouldCancel)
        {
          return scanResult;
        }
        if(!next.has_value())
        {
          break;
        }
        uint64 occurrenceCount = 0;
        bool occurrenceOverflow = false;
        scanResult = scanGroup(group, [&](const T& value) {
          if(EquivalentStatisticsValues(value, *next))
          {
            if(occurrenceCount == std::numeric_limits<uint64>::max())
            {
              occurrenceOverflow = true;
            }
            else
            {
              ++occurrenceCount;
            }
          }
        });
        if(scanResult.invalid() || shouldCancel)
        {
          return scanResult;
        }
        if(occurrenceOverflow)
        {
          return MakeErrorResult(-57304, fmt::format("ComputeArrayStatistics: occurrence count for output group {} exceeds uint64.", group));
        }
        Result<> consumeResult = consumeSortedRun(group, *next, occurrenceCount, groupPosition);
        if(consumeResult.invalid())
        {
          return consumeResult;
        }
        groupPosition += occurrenceCount;
        previous = next;
      }
    }
  }

  if(inputValues.find_median)
  {
    for(usize group = 0; group < groupCount; ++group)
    {
      if(lengths[group] == 0)
      {
        continue;
      }
      if(hasMedianLower[group] == 0 || hasMedianUpper[group] == 0)
      {
        return MakeErrorResult(-57313, fmt::format("ComputeArrayStatistics: exact median positions were not found for non-empty output group {}.", group));
      }
      if(lengths[group] % 2 == 1)
      {
        medians[group] = static_cast<float32>(medianUpper[group]);
      }
      else
      {
        medians[group] = static_cast<float32>((medianLower[group] + medianUpper[group]) * 0.5F);
      }
    }
  }

  // Mode ties require a second ordered traversal after maximum run sizes are known.
  if constexpr(!std::is_same_v<T, bool>)
  {
    if(inputValues.find_mode)
    {
      auto* modeArrayPtr = dataStructure.getDataAs<NeighborList<T>>(inputValues.mode_array_name);
      if(shouldCancel)
      {
        return {};
      }
      try
      {
        modeArrayPtr->clearAllLists();
        Result<> resizeResult = modeArrayPtr->resizeTuples({groupCount});
        if(resizeResult.invalid())
        {
          for(auto& error : resizeResult.errors())
          {
            error.message = fmt::format("ComputeArrayStatistics: could not resize Mode output '{}' to {} tuples: {}", inputValues.mode_array_name.toString(), groupCount, error.message);
          }
          return resizeResult;
        }
      } catch(const std::exception& exception)
      {
        return MakeErrorResult(-57309, fmt::format("ComputeArrayStatistics: could not initialize Mode output '{}': {}", inputValues.mode_array_name.toString(), exception.what()));
      }
      const auto appendMode = [&](usize group, const T& value) -> Result<> {
        if(shouldCancel)
        {
          return {};
        }
        try
        {
          modeArrayPtr->addEntry(static_cast<int32>(group), value);
        } catch(const std::exception& exception)
        {
          return MakeErrorResult(-57314,
                                 fmt::format("ComputeArrayStatistics: could not append a Mode value to output group {} in '{}': {}", group, inputValues.mode_array_name.toString(), exception.what()));
        }
        return {};
      };
      if(externalSort != nullptr)
      {
        Result<> modeResult = ScanSortedStatisticsRuns<T>(*externalSort, groupCount, shouldCancel, [&](usize group, const T& value, uint64 runCount, uint64) -> Result<> {
          if(runCount == maximumModeCounts[group])
          {
            return appendMode(group, value);
          }
          return {};
        });
        if(modeResult.invalid() || shouldCancel)
        {
          return modeResult;
        }
      }
      else
      {
        const auto scanGroup = [&](usize requestedGroup, auto&& valueFunction) -> Result<> {
          for(usize offset = 0; offset < tupleCount; offset += k_ChunkTuples)
          {
            if(shouldCancel)
            {
              return {};
            }
            const usize count = std::min(k_ChunkTuples, tupleCount - offset);
            Result<> readResult = readChunk(offset, count);
            if(readResult.invalid())
            {
              return readResult;
            }
            if(shouldCancel)
            {
              return {};
            }
            for(usize index = 0; index < count; ++index)
            {
              if(maskStorePtr != nullptr && !static_cast<bool>(maskBuffer[index]))
              {
                continue;
              }
              const std::optional<usize> currentGroup = layout.groupIndex(featureIdsStorePtr == nullptr ? 0 : featureBuffer[index]);
              if(currentGroup.has_value() && *currentGroup == requestedGroup)
              {
                valueFunction(valueBuffer[index]);
              }
            }
          }
          return {};
        };
        for(usize group = 0; group < groupCount; ++group)
        {
          std::optional<T> previous;
          while(!shouldCancel)
          {
            std::optional<T> next;
            Result<> scanResult = scanGroup(group, [&](const T& value) {
              if((!previous.has_value() || *previous < value) && (!next.has_value() || value < *next))
              {
                next = value;
              }
            });
            if(scanResult.invalid() || shouldCancel)
            {
              return scanResult;
            }
            if(!next.has_value())
            {
              break;
            }
            uint64 occurrenceCount = 0;
            bool occurrenceOverflow = false;
            scanResult = scanGroup(group, [&](const T& value) {
              if(EquivalentStatisticsValues(value, *next))
              {
                if(occurrenceCount == std::numeric_limits<uint64>::max())
                {
                  occurrenceOverflow = true;
                }
                else
                {
                  ++occurrenceCount;
                }
              }
            });
            if(scanResult.invalid() || shouldCancel)
            {
              return scanResult;
            }
            if(occurrenceOverflow)
            {
              return MakeErrorResult(-57304, fmt::format("ComputeArrayStatistics: occurrence count for output group {} exceeds uint64.", group));
            }
            if(occurrenceCount == maximumModeCounts[group])
            {
              Result<> appendResult = appendMode(group, *next);
              if(appendResult.invalid())
              {
                return appendResult;
              }
            }
            previous = next;
          }
        }
      }
    }
  }

  if(shouldCancel)
  {
    return {};
  }
  // Feature-scale scalar outputs commit in bulk after the reductions succeed.
  if(inputValues.compute_by_index)
  {
    auto hasData = std::make_unique<bool[]>(groupCount);
    for(usize group = 0; group < groupCount; ++group)
    {
      hasData[group] = lengths[group] > 0;
    }
    Result<> writeResult = WriteStatisticsOutput(featureHasDataArrayPtr, hasData.get(), groupCount, inputValues.feature_has_data_array_name);
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  if(shouldCancel)
  {
    return {};
  }
  if(inputValues.find_length)
  {
    Result<> writeResult = WriteStatisticsOutput(lengthArrayPtr, lengths.data(), groupCount, inputValues.length_array_name);
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  if(shouldCancel)
  {
    return {};
  }
  if(inputValues.find_min)
  {
    Result<> writeResult = WriteStatisticsOutput(minimumArrayPtr, minimums.get(), groupCount, inputValues.minimum_array_name);
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  if(shouldCancel)
  {
    return {};
  }
  if(inputValues.find_max)
  {
    Result<> writeResult = WriteStatisticsOutput(maximumArrayPtr, maximums.get(), groupCount, inputValues.maximum_array_name);
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  if(shouldCancel)
  {
    return {};
  }
  if(inputValues.find_mean)
  {
    Result<> writeResult = WriteStatisticsOutput(meanArrayPtr, means.data(), groupCount, inputValues.mean_array_name);
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  if(shouldCancel)
  {
    return {};
  }
  if(inputValues.find_median)
  {
    Result<> writeResult = WriteStatisticsOutput(medianArrayPtr, medians.data(), groupCount, inputValues.median_array_name);
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  if(shouldCancel)
  {
    return {};
  }
  if(inputValues.find_std_deviation)
  {
    Result<> writeResult = WriteStatisticsOutput(stdDeviationArrayPtr, standardDeviations.data(), groupCount, inputValues.std_deviation_array_name);
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  if(shouldCancel)
  {
    return {};
  }
  if(inputValues.find_summation)
  {
    Result<> writeResult;
    if constexpr(std::is_same_v<T, bool>)
    {
      std::vector<float32> summationValues(groupCount);
      std::transform(sums.cbegin(), sums.cend(), summationValues.begin(), [](uint64 count) { return static_cast<float32>(count); });
      writeResult = WriteStatisticsOutput(summationArrayPtr, summationValues.data(), groupCount, inputValues.summation_array_name);
    }
    else
    {
      writeResult = WriteStatisticsOutput(summationArrayPtr, sums.data(), groupCount, inputValues.summation_array_name);
    }
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  if(shouldCancel)
  {
    return {};
  }
  if(inputValues.find_num_unique_values)
  {
    Result<> writeResult = WriteStatisticsOutput(uniqueValuesArrayPtr, uniqueCounts.data(), groupCount, inputValues.num_unique_values_name);
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }

  if(shouldCancel)
  {
    return {};
  }
  // Standardization uses final means and deviations in fixed read-modify-write pages.
  if(inputValues.standardize_data)
  {
    auto standardizedBuffer = std::make_unique<float32[]>(k_ChunkTuples);
    auto& standardizedStore = standardizedArrayPtr->getDataStoreRef();
    for(usize offset = 0; offset < tupleCount; offset += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_ChunkTuples, tupleCount - offset);
      Result<> readResult = readChunk(offset, count);
      if(readResult.invalid())
      {
        return readResult;
      }
      if(shouldCancel)
      {
        return {};
      }
      readResult = standardizedStore.copyIntoBuffer(offset, nonstd::span<float32>(standardizedBuffer.get(), count));
      if(readResult.invalid())
      {
        return readResult;
      }
      for(usize index = 0; index < count; ++index)
      {
        if(maskStorePtr != nullptr && !static_cast<bool>(maskBuffer[index]))
        {
          continue;
        }
        const std::optional<usize> group = layout.groupIndex(featureIdsStorePtr == nullptr ? 0 : featureBuffer[index]);
        if(!group.has_value())
        {
          continue;
        }
        standardizedBuffer[index] = (static_cast<float32>(valueBuffer[index]) - means[*group]) / standardDeviations[*group];
      }
      Result<> writeResult = standardizedStore.copyFromBuffer(offset, nonstd::span<const float32>(standardizedBuffer.get(), count));
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }
  }
  return {};
}

/**
 * @struct StatisticsScanlineFunctor
 * @brief Dispatches bounded statistics by input and mask type.
 */
struct StatisticsScanlineFunctor
{
  /**
   * @brief Selects an absent, Boolean, or UInt8 mask for a bounded scan.
   * @tparam T Input and extrema value type.
   * @param dataStructure Owns input and output arrays.
   * @param inputArray Input values.
   * @param featureIdsArray Optional FeatureIds array.
   * @param maskArray Optional mask array.
   * @param layout Resolved compact output layout.
   * @param inputValues Selected statistics and output paths.
   * @param shouldCancel Cancellation flag.
   * @return Result from the typed bounded scan.
   */
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const IDataArray& inputArray, const IDataArray* featureIdsArray, const IDataArray* maskArray, const StatisticsGroupLayout& layout,
                      const ComputeArrayStatisticsInputValues& inputValues, const std::atomic_bool& shouldCancel) const
  {
    if(maskArray == nullptr)
    {
      return GenerateScanlineStatistics<T, uint8>(dataStructure, inputArray, featureIdsArray, nullptr, layout, inputValues, shouldCancel);
    }
    if(maskArray->getDataType() == DataType::boolean)
    {
      return GenerateScanlineStatistics<T, bool>(dataStructure, inputArray, featureIdsArray, maskArray, layout, inputValues, shouldCancel);
    }
    if(maskArray->getDataType() == DataType::uint8)
    {
      return GenerateScanlineStatistics<T, uint8>(dataStructure, inputArray, featureIdsArray, maskArray, layout, inputValues, shouldCancel);
    }
    return MakeErrorResult(-57315, fmt::format("ComputeArrayStatistics: mask '{}' must have Boolean or UInt8 values.", maskArray->getName()));
  }
};

/**
 * @class ComputeArrayStatisticsDirect
 * @brief Invokes the existing resident statistics implementation.
 */
class ComputeArrayStatisticsDirect
{
public:
  /**
   * @brief Stores a resident callback for synchronous dispatch.
   * @tparam ArgsT Additional dispatch argument types.
   * @param executeDirect Resident implementation callback.
   * @param args Forwarded arguments ignored by this wrapper.
   * @pre executeDirect outlives this wrapper.
   */
  template <typename... ArgsT>
  explicit ComputeArrayStatisticsDirect(const std::function<Result<>()>& executeDirect, ArgsT&&... args)
  : m_ExecuteDirect(executeDirect)
  {
  }

  /**
   * @brief Executes the resident implementation callback.
   * @return Callback result.
   */
  Result<> operator()() const
  {
    return m_ExecuteDirect();
  }

private:
  const std::function<Result<>()>& m_ExecuteDirect;
};

/**
 * @class ComputeArrayStatisticsScanline
 * @brief Stores borrowed inputs for bounded statistics.
 *
 * All captured references are non-owning and used synchronously by operator().
 */
class ComputeArrayStatisticsScanline
{
public:
  /**
   * @brief Stores resolved layout, source arrays, and execution controls.
   * @param executeDirect Resident callback ignored by the scanline wrapper.
   * @param dataStructure Owns input and output arrays.
   * @param inputArray Input values.
   * @param featureIdsArray Optional FeatureIds array.
   * @param maskArray Optional mask array.
   * @param layout Resolved compact output layout.
   * @param inputValues Selected statistics and output paths.
   * @param shouldCancel Cancellation flag.
   * @pre Referenced arrays, layout, inputValues, and shouldCancel outlive this wrapper.
   */
  ComputeArrayStatisticsScanline(const std::function<Result<>()>& executeDirect, DataStructure& dataStructure, const IDataArray& inputArray, const IDataArray* featureIdsArray,
                                 const IDataArray* maskArray, const StatisticsGroupLayout& layout, const ComputeArrayStatisticsInputValues& inputValues, const std::atomic_bool& shouldCancel)
  : m_DataStructure(dataStructure)
  , m_InputArray(inputArray)
  , m_FeatureIdsArray(featureIdsArray)
  , m_MaskArray(maskArray)
  , m_Layout(layout)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Dispatches bounded statistics by input value type.
   * @return Result from the typed bounded scan.
   */
  Result<> operator()() const
  {
    return ExecuteDataFunction(StatisticsScanlineFunctor{}, m_InputArray.getDataType(), m_DataStructure, m_InputArray, m_FeatureIdsArray, m_MaskArray, m_Layout, m_InputValues, m_ShouldCancel);
  }

private:
  DataStructure& m_DataStructure;
  const IDataArray& m_InputArray;
  const IDataArray* m_FeatureIdsArray = nullptr;
  const IDataArray* m_MaskArray = nullptr;
  const StatisticsGroupLayout& m_Layout;
  const ComputeArrayStatisticsInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

ComputeArrayStatistics::ComputeArrayStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeArrayStatisticsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
, m_Throttle(m_MessageHandler)
{
}

ComputeArrayStatistics::~ComputeArrayStatistics() noexcept = default;

Result<> ComputeArrayStatistics::operator()()
{
  if(!m_InputValues->find_min && !m_InputValues->find_max && !m_InputValues->find_mean && !m_InputValues->find_median && !m_InputValues->find_mode && !m_InputValues->find_std_deviation &&
     !m_InputValues->find_summation && !m_InputValues->find_length && !m_InputValues->find_num_unique_values)
  {
    return {};
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  const auto* inputArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->selected_array_path);
  if(inputArrayPtr == nullptr)
  {
    return MakeErrorResult(-57316, fmt::format("ComputeArrayStatistics: input array '{}' does not exist.", m_InputValues->selected_array_path.toString()));
  }
  const auto* featureIdsArrayPtr = m_InputValues->compute_by_index ? m_DataStructure.getDataAs<Int32ArrayType>(m_InputValues->feature_ids_array_path) : nullptr;
  if(m_InputValues->compute_by_index && featureIdsArrayPtr == nullptr)
  {
    return MakeErrorResult(-57317, fmt::format("ComputeArrayStatistics: FeatureIds array '{}' does not exist or is not Int32.", m_InputValues->feature_ids_array_path.toString()));
  }
  const auto* maskArrayPtr = m_InputValues->use_mask ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->mask_array_path) : nullptr;
  if(m_InputValues->use_mask && (maskArrayPtr == nullptr || (maskArrayPtr->getDataType() != DataType::boolean && maskArrayPtr->getDataType() != DataType::uint8)))
  {
    return MakeErrorResult(-57315, fmt::format("ComputeArrayStatistics: mask '{}' does not exist or is not Boolean/UInt8.", m_InputValues->mask_array_path.toString()));
  }

  // Discover FeatureId bounds before destination resizing so entry cancellation preserves outputs.
  ObservedFeatureBounds observedBounds;
  if(featureIdsArrayPtr != nullptr)
  {
    Result<ObservedFeatureBounds> boundsResult = DiscoverFeatureBounds(featureIdsArrayPtr->getDataStoreRef(), m_ShouldCancel);
    if(boundsResult.invalid())
    {
      return ConvertResult(std::move(boundsResult));
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    observedBounds = boundsResult.value();
  }
  Result<StatisticsGroupLayout> layoutResult = ResolveGroupLayout(*m_InputValues, observedBounds);
  if(layoutResult.invalid())
  {
    return ConvertResult(std::move(layoutResult));
  }
  const StatisticsGroupLayout layout = layoutResult.value();
  if(m_ShouldCancel)
  {
    return {};
  }

  if(layout.compute_by_index)
  {
    auto* destinationPtr = m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->destination_attribute_matrix);
    if(destinationPtr == nullptr)
    {
      return MakeErrorResult(-57318, fmt::format("ComputeArrayStatistics: destination AttributeMatrix '{}' does not exist.", m_InputValues->destination_attribute_matrix.toString()));
    }
    try
    {
      Result<> resizeResult = destinationPtr->resizeTuples({layout.group_count});
      if(resizeResult.invalid())
      {
        return resizeResult;
      }
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-57319, fmt::format("ComputeArrayStatistics: could not resize destination AttributeMatrix '{}' to {} tuples: {}", m_InputValues->destination_attribute_matrix.toString(),
                                                 layout.group_count, exception.what()));
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    if(layout.create_feature_id_map)
    {
      auto* featureIdMapPtr = m_DataStructure.getDataAs<Int32ArrayType>(m_InputValues->feature_id_map_array_path);
      if(featureIdMapPtr == nullptr)
      {
        return MakeErrorResult(-57320, fmt::format("ComputeArrayStatistics: FeatureId mapping output '{}' does not exist or is not Int32.", m_InputValues->feature_id_map_array_path.toString()));
      }
      // The compact-to-source map scales with output groups, not cell tuples.
      std::vector<int32> mapping(layout.group_count);
      for(usize index = 0; index < layout.group_count; ++index)
      {
        if(index % k_ChunkTuples == 0 && m_ShouldCancel)
        {
          return {};
        }
        mapping[index] = static_cast<int32>(static_cast<int64>(layout.first_feature_id) + static_cast<int64>(index));
      }
      if(m_ShouldCancel)
      {
        return {};
      }
      Result<> mapWriteResult = featureIdMapPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(mapping.data(), mapping.size()));
      if(mapWriteResult.invalid())
      {
        return mapWriteResult;
      }
      if(m_ShouldCancel)
      {
        return {};
      }
    }
  }

  std::vector<IArray*> arrays(10, nullptr);
  arrays[0] = m_InputValues->find_length ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->length_array_name) : nullptr;
  arrays[1] = m_InputValues->find_min ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->minimum_array_name) : nullptr;
  arrays[2] = m_InputValues->find_max ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->maximum_array_name) : nullptr;
  arrays[3] = m_InputValues->find_mean ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->mean_array_name) : nullptr;
  arrays[4] = m_InputValues->find_median ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->median_array_name) : nullptr;
  arrays[5] = m_InputValues->find_mode ? m_DataStructure.getDataAs<INeighborList>(m_InputValues->mode_array_name) : nullptr;
  arrays[6] = m_InputValues->find_std_deviation ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->std_deviation_array_name) : nullptr;
  arrays[7] = m_InputValues->find_summation ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->summation_array_name) : nullptr;
  arrays[8] = m_InputValues->find_num_unique_values ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->num_unique_values_name) : nullptr;
  arrays[9] = m_InputValues->compute_by_index ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->feature_has_data_array_name) : nullptr;

  const std::function<Result<>()> executeDirect = [&]() -> Result<> {
    if(inputArrayPtr->getDataType() == DataType::boolean)
    {
      return ExecuteDataFunction(StatisticsScanlineFunctor{}, inputArrayPtr->getDataType(), m_DataStructure, *inputArrayPtr, featureIdsArrayPtr, maskArrayPtr, layout, *m_InputValues, m_ShouldCancel);
    }
    if(!m_InputValues->compute_by_index)
    {
      if(!m_InputValues->find_mode)
      {
        return ExecuteDataFunctionNoBool(ComputeArrayStatisticsFunctor{}, inputArrayPtr->getDataType(), m_DataStructure, *inputArrayPtr, arrays, m_InputValues);
      }
      return ExecuteNeighborFunction(ComputeArrayStatisticsFunctor{}, inputArrayPtr->getDataType(), m_DataStructure, *inputArrayPtr, arrays, m_InputValues);
    }
    if(layout.group_count == 0)
    {
      return ExecuteDataFunction(StatisticsScanlineFunctor{}, inputArrayPtr->getDataType(), m_DataStructure, *inputArrayPtr, featureIdsArrayPtr, maskArrayPtr, layout, *m_InputValues, m_ShouldCancel);
    }
    if(m_InputValues->range_type == to_underlying(FeatureIdRangeControls::None))
    {
      if(!m_InputValues->find_mode)
      {
        return ExecuteDataFunctionNoBool(ComputeArrayStatisticsByFeatureFunctor{}, inputArrayPtr->getDataType(), m_DataStructure, inputArrayPtr, arrays, layout.group_count, m_InputValues,
                                         m_ShouldCancel, *this);
      }
      return ExecuteNeighborFunction(ComputeArrayStatisticsByFeatureFunctor{}, inputArrayPtr->getDataType(), m_DataStructure, inputArrayPtr, arrays, layout.group_count, m_InputValues, m_ShouldCancel,
                                     *this);
    }
    if(!m_InputValues->find_mode)
    {
      return ExecuteDataFunctionNoBool(ComputeArrayStatisticsByFeatureFunctor{}, inputArrayPtr->getDataType(), m_DataStructure, inputArrayPtr, arrays,
                                       std::make_pair(layout.first_feature_id, layout.last_feature_id), m_InputValues, m_ShouldCancel);
    }
    return ExecuteNeighborFunction(ComputeArrayStatisticsByFeatureFunctor{}, inputArrayPtr->getDataType(), m_DataStructure, inputArrayPtr, arrays,
                                   std::make_pair(layout.first_feature_id, layout.last_feature_id), m_InputValues, m_ShouldCancel);
  };

  // Every input and enabled output selects the safe storage implementation.
  std::vector<const IArray*> targets;
  targets.reserve(14);
  targets.push_back(inputArrayPtr);
  if(featureIdsArrayPtr != nullptr)
  {
    targets.push_back(featureIdsArrayPtr);
  }
  if(maskArrayPtr != nullptr)
  {
    targets.push_back(maskArrayPtr);
  }
  for(IArray* outputPtr : arrays)
  {
    if(outputPtr != nullptr)
    {
      targets.push_back(outputPtr);
    }
  }
  if(layout.create_feature_id_map)
  {
    targets.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->feature_id_map_array_path));
  }
  if(m_InputValues->standardize_data)
  {
    targets.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->standardized_array_name));
  }

  return DispatchAlgorithm<ComputeArrayStatisticsDirect, ComputeArrayStatisticsScanline>(AlgorithmArrayTargets(std::move(targets)), executeDirect, m_DataStructure, *inputArrayPtr, featureIdsArrayPtr,
                                                                                         maskArrayPtr, layout, *m_InputValues, m_ShouldCancel);
}

void ComputeArrayStatistics::sendThreadSafeInfoMessage(const std::string& message)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessageMutex);
  m_MessageHandler.sendInfoMessage(message);
}

void ComputeArrayStatistics::resetProgress(usize totalWork)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessageMutex);
  m_Throttle.reset(totalWork, "Computing Feature Statistics");
}

void ComputeArrayStatistics::sendThreadSafeProgressMessage(usize completedWork)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessageMutex);
  m_Throttle.incrementPercent(completedWork);
}
