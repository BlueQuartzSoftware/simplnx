#include "ComputeArrayStatistics.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/HistogramUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/Math/StatisticsCalculations.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <algorithm>
#include <unordered_map>

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
bool CheckArraysInMemory(const nx::core::IParallelAlgorithm::AlgorithmArrays& arrays)
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

    if(!arrayPtr->getIDataStoreRef().getDataFormat().empty())
    {
      return false;
    }
  }

  return true;
}

template <typename T>
class StatisticsByFeatureImpl
{
public:
  StatisticsByFeatureImpl(bool length, bool min, bool max, bool mean, bool mode, bool stdDeviation, bool summation, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask,
                          const Int32AbstractDataStore& featureIds, const AbstractDataStore<T>& source, BoolArray* featureHasDataArray, UInt64Array* lengthArray, DataArray<T>* minArray,
                          DataArray<T>* maxArray, Float32Array* meanArray, NeighborList<T>* modeArray, Float32Array* stdDevArray, Float32Array* summationArray, const std::atomic_bool& shouldCancel,
                          ComputeArrayStatistics* filter)
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
  , m_Filter(filter)
  {
  }

  void compute(usize start, usize end) const
  {

    const usize numTuples = m_FeatureIds.getNumberOfTuples();
    const usize numCurrentFeatures = end - start;

    auto msgHandler = [this](const std::string& msg) { m_Filter->sendThreadSafeProgressMessage([&] { return "Preparing features/ensembles for stats calculation " + msg; }); };
    auto [length, min, max, summation, modalMaps] = HistogramUtilities::concurrent::CalculateFeatureHasDataStats(m_Source, m_FeatureIds, start, end, m_Mask, msgHandler, m_ShouldCancel);
    if(m_ShouldCancel)
    {
      return;
    }

    usize progressCount = 0;
    usize progressIncrement = numCurrentFeatures / 100;

    m_Filter->sendThreadSafeInfoMessage(fmt::format("Calculating statistics for feature range [{}-{}]", start, end));

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
        if constexpr(std::is_same_v<T, bool>)
        {
          meanValue = static_cast<float32>(summation[localFeatureIndex] >= (numTuples - summation[localFeatureIndex]));
        }
        else
        {
          meanValue = summation[localFeatureIndex] / static_cast<float32>(length[localFeatureIndex]);
        }
      }

      if(m_Mean)
      {
        m_MeanArray->initializeTuple(j, meanValue);
      }
      else if(m_StdDeviation)
      {
        meanArray[localFeatureIndex] = meanValue;
      }

      if(m_Mode)
      {
        if(!modalMaps[localFeatureIndex].empty())
        {
          // Find the maximum occurrence
          auto pr = std::max_element(modalMaps[localFeatureIndex].begin(), modalMaps[localFeatureIndex].end(), [](const auto& x, const auto& y) { return x.second < y.second; });
          int maxCount = pr->second;

          // Store all values that have this maximum occurrence under the proper feature id
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
      if(progressCount > progressIncrement)
      {
        m_Filter->sendThreadSafeProgressMessage([&]() {
          progressCount = 0;
          return fmt::format("Calculating statistics for feature [{}-{}] {}/{}", start, end, j, end);
        });
      }
    }

    if(m_StdDeviation)
    {
      // https://www.khanacademy.org/math/statistics-probability/summarizing-quantitative-data/variance-standard-deviation-population/a/calculating-standard-deviation-step-by-step
      m_Filter->sendThreadSafeInfoMessage(fmt::format("Computing StdDev Feature/Ensemble [{}-{}]", start, end));
      // This should probably be done with Kahan Summation instead
      std::vector<float64> sumOfDiffs(numCurrentFeatures, 0.0f);
      progressCount = 0;

      for(usize tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
      {
        if(m_ShouldCancel)
        {
          return;
        }
        // Is the value in a mask and if so, is that mask TRUE
        if(m_Mask != nullptr && !m_Mask->isTrue(tupleIndex))
        {
          continue;
        }
        // Is the featureId within our range that we care about
        const int32 featureId = m_FeatureIds[tupleIndex];
        if(featureId < start || featureId >= end)
        {
          continue;
        }

        const float32 meanVal = m_Mean ? m_MeanArray->operator[](featureId) : meanArray[featureId - start];
        sumOfDiffs[featureId - start] += static_cast<float64>((m_Source[tupleIndex] - meanVal) * (m_Source[tupleIndex] - meanVal));

        progressCount++;
        if(progressCount > progressIncrement)
        {
          m_Filter->sendThreadSafeProgressMessage([&]() {
            progressCount = 0;
            return fmt::format("StdDev Calculation Feature/Ensemble [{}-{}]: {:.2f}%", start, end, 100.0f * static_cast<float>(tupleIndex) / static_cast<float>(numTuples));
          });
        }
      }

      for(usize j = 0; j < numCurrentFeatures; j++)
      {
        // Set the value into the output array
        const uint64 lengthVal = m_Length ? m_LengthArray->operator[](j + start) : length[j];
        m_StdDevArray->operator[](j + start) = static_cast<float32>(std::sqrt(sumOfDiffs[j] / static_cast<float64>(lengthVal)));
      }
    }

  } // end of compute

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
  const std::unique_ptr<MaskCompareUtilities::MaskCompare>& m_Mask = nullptr;
  const Int32AbstractDataStore& m_FeatureIds;
  const AbstractDataStore<T>& m_Source;
  BoolArray* m_FeatureHasDataArray = nullptr;
  UInt64Array* m_LengthArray = nullptr;
  DataArray<T>* m_MinArray = nullptr;
  DataArray<T>* m_MaxArray = nullptr;
  Float32Array* m_MeanArray = nullptr;
  NeighborList<T>* m_ModeArray = nullptr;
  Float32Array* m_StdDevArray = nullptr;
  Float32Array* m_SummationArray = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  ComputeArrayStatistics* m_Filter = nullptr;
};

template <typename T>
class StatisticsByFeatureRangeImpl
{
public:
  StatisticsByFeatureRangeImpl(bool length, bool min, bool max, bool mean, bool mode, bool stdDeviation, bool summation, const Int32AbstractDataStore& featureIdsMap,
                               const BoolAbstractDataStore& tempMask, const Int32AbstractDataStore& featureIds, const AbstractDataStore<T>& source, BoolArray* featureHasDataArray,
                               UInt64Array* lengthArray, DataArray<T>* minArray, DataArray<T>* maxArray, Float32Array* meanArray, NeighborList<T>* modeArray, Float32Array* stdDevArray,
                               Float32Array* summationArray, const std::atomic_bool& shouldCancel, ComputeArrayStatistics* filter)
  : m_Length(length)
  , m_Min(min)
  , m_Max(max)
  , m_Mean(mean)
  , m_Mode(mode)
  , m_StdDeviation(stdDeviation)
  , m_Summation(summation)
  , m_FeatureIdsMap(featureIdsMap)
  , m_Mask(tempMask)
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
  , m_Filter(filter)
  {
  }

  void compute(usize start, usize end) const
  {
    const usize numTuples = m_Source.getNumberOfTuples();
    for(usize featureId = start; featureId < end; featureId++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      const usize truePosition = std::distance(m_FeatureIdsMap.begin(), std::find(m_FeatureIdsMap.begin(), m_FeatureIdsMap.end(), featureId));
      T minValue = std::numeric_limits<T>::max();
      T maxValue;
      if constexpr(std::is_floating_point_v<T>)
      {
        maxValue = -std::numeric_limits<T>::max();
      }
      else
      {
        maxValue = std::numeric_limits<T>::min();
      }
      T summationValue = static_cast<T>(0);
      float32 meanValue = 0.0;
      usize count = 0;
      std::map<T, uint64> modalMap = {};
      for(usize i = 0; i < numTuples; ++i)
      {
        if(m_Mask[i] && m_FeatureIds[i] == featureId)
        {
          count++;
          T val = m_Source[i];
          minValue = std::min(minValue, val);
          maxValue = std::max(maxValue, val);
          summationValue += val;
          modalMap[val]++;
        }
      }

      if(count > 0) // This guards against dividing by zero
      {
        m_FeatureHasDataArray->initializeTuple(truePosition, true);
        if(m_Length)
        {
          m_LengthArray->initializeTuple(truePosition, count);
        }
        if(m_Min)
        {
          m_MinArray->initializeTuple(truePosition, minValue);
        }
        if(m_Max)
        {
          m_MaxArray->initializeTuple(truePosition, maxValue);
        }
        if(m_Summation)
        {
          m_SummationArray->initializeTuple(truePosition, summationValue);
        }

        if(count > 0)
        {
          if constexpr(std::is_same_v<T, bool>)
          {
            meanValue = static_cast<float32>(summationValue >= (numTuples - summationValue));
          }
          else
          {
            meanValue = summationValue / static_cast<float32>(count);
          }
        }

        if(m_Mean)
        {
          m_MeanArray->initializeTuple(truePosition, meanValue);
        }

        if(m_Mode)
        {
          if(!modalMap.empty())
          {
            // Find the maximum occurrence
            auto pr = std::max_element(modalMap.begin(), modalMap.end(), [](const auto& x, const auto& y) { return x.second < y.second; });
            int maxCount = pr->second;

            // Store all values that have this maximum occurrence under the proper feature id
            for(const auto& modalPair : modalMap)
            {
              if(modalPair.second == maxCount)
              {
                m_ModeArray->addEntry(truePosition, modalPair.first);
              }
            }
          }
        }

        if(m_StdDeviation)
        {
          // https://www.khanacademy.org/math/statistics-probability/summarizing-quantitative-data/variance-standard-deviation-population/a/calculating-standard-deviation-step-by-step
          float64 sumOfDiffs = 0.0;
          for(usize i = 0; i < numTuples; ++i)
          {
            if(m_Mask[i] && m_FeatureIds[i] == featureId)
            {
              sumOfDiffs += static_cast<float64>((m_Source[i] - meanValue) * (m_Source[i] - meanValue));
            }
          }

          // Set the value into the output array
          m_StdDevArray->setValue(truePosition, static_cast<float32>(std::sqrt(sumOfDiffs / static_cast<float64>(count))));
        }
      }
      else
      {
        m_FeatureHasDataArray->initializeTuple(truePosition, false);
      }

      m_Filter->sendThreadSafeProgressMessage([&]() { return fmt::format("Storing data for feature/ensembles [{}-{}] {}/{}", start, end, featureId, end); });
    }
  } // end of compute

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
  const Int32AbstractDataStore& m_FeatureIdsMap;
  const BoolAbstractDataStore& m_Mask;
  const Int32AbstractDataStore& m_FeatureIds;
  const AbstractDataStore<T>& m_Source;
  BoolArray* m_FeatureHasDataArray = nullptr;
  UInt64Array* m_LengthArray = nullptr;
  DataArray<T>* m_MinArray = nullptr;
  DataArray<T>* m_MaxArray = nullptr;
  Float32Array* m_MeanArray = nullptr;
  NeighborList<T>* m_ModeArray = nullptr;
  Float32Array* m_StdDevArray = nullptr;
  Float32Array* m_SummationArray = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  ComputeArrayStatistics* m_Filter = nullptr;
};

template <typename T>
class MedianByFeatureImpl
{
public:
  MedianByFeatureImpl(const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, const Int32AbstractDataStore& featureIds, const AbstractDataStore<T>& source, bool findMedian, bool findNumUnique,
                      Float32Array* medianArray, Int32Array* numUniqueValuesArray, DataArray<uint64>* lengthArray, ComputeArrayStatistics* filter)
  : m_FindMedian(findMedian)
  , m_FindNumUniqueValues(findNumUnique)
  , m_MedianArray(medianArray)
  , m_NumUniqueValuesArray(numUniqueValuesArray)
  , m_Mask(mask)
  , m_FeatureIds(featureIds)
  , m_Source(source)
  , m_LengthArray(lengthArray)
  , m_Filter(filter)
  {
  }

  void compute(usize start, usize end) const
  {
    m_Filter->sendThreadSafeInfoMessage(fmt::format("Starting Median Array Calculation: Feature/Ensemble [{}-{}]", start, end));

    const usize numFeatureSources = end - start;
    // Create the arrays that will collect the values from the arrays. allocate them to the correct size based on the length array
    std::vector<std::vector<T>> featureSources(numFeatureSources);
    for(usize featureSourceIndex = 0; featureSourceIndex < numFeatureSources; featureSourceIndex++)
    {
      featureSources[featureSourceIndex].reserve(m_LengthArray->operator[](featureSourceIndex + start));
    }
    const usize numTuples = m_Source.getNumberOfTuples();

    for(usize tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
    {
      // Is the value in a mask and if so, is that mask TRUE
      if(m_Mask != nullptr && !m_Mask->isTrue(tupleIndex))
      {
        continue;
      }
      // Is the featureId within our range that we care about
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
        const float32 val = StatisticsCalculations::findMedian(featureSources[featureSourceIndex]);
        m_MedianArray->setValue(featureSourceIndex + start, val);
      }
      if(m_FindNumUniqueValues)
      {
        const auto val = StatisticsCalculations::findNumUniqueValues(featureSources[featureSourceIndex]);
        m_NumUniqueValuesArray->setValue(featureSourceIndex + start, val);
      }
    }
  }

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  bool m_FindMedian;
  bool m_FindNumUniqueValues;
  Float32Array* m_MedianArray;
  Int32Array* m_NumUniqueValuesArray;
  const std::unique_ptr<MaskCompareUtilities::MaskCompare>& m_Mask = nullptr;
  const Int32AbstractDataStore& m_FeatureIds;
  const AbstractDataStore<T>& m_Source;
  const DataArray<uint64>* m_LengthArray = nullptr;
  ComputeArrayStatistics* m_Filter = nullptr;
};

template <typename T>
class MedianByFeatureRangeImpl
{
public:
  MedianByFeatureRangeImpl(const Int32AbstractDataStore& featureIdsMap, const BoolAbstractDataStore& tempMask, const Int32AbstractDataStore& featureIds, const AbstractDataStore<T>& source,
                           bool findMedian, bool findNumUnique, Float32Array* medianArray, Int32Array* numUniqueValuesArray, ComputeArrayStatistics* filter)
  : m_FindMedian(findMedian)
  , m_FindNumUniqueValues(findNumUnique)
  , m_MedianArray(medianArray)
  , m_NumUniqueValuesArray(numUniqueValuesArray)
  , m_FeatureIdsMap(featureIdsMap)
  , m_Mask(tempMask)
  , m_FeatureIds(featureIds)
  , m_Source(source)
  , m_Filter(filter)
  {
  }

  void compute(usize start, usize end) const
  {
    m_Filter->sendThreadSafeInfoMessage(fmt::format("Starting Median Array Calculation: Feature/Ensemble [{}-{}]", start, end));

    const usize numTuples = m_Source.getNumberOfTuples();
    for(usize featureId = start; featureId < end; featureId++)
    {
      const usize truePosition = std::distance(m_FeatureIdsMap.begin(), std::find(m_FeatureIdsMap.begin(), m_FeatureIdsMap.end(), featureId));
      std::set<int32> valuesSet = {};
      std::vector<float32> values = {};
      for(usize i = 0; i < numTuples; i++)
      {
        if(m_Mask[i] && m_FeatureIds[i] == featureId)
        {
          if(m_FindMedian)
          {
            values.push_back(static_cast<float>(m_Source[i]));
          }
          if(m_FindNumUniqueValues)
          {
            valuesSet.emplace(static_cast<int32>(m_Source[i]));
          }
        }
      }

      if(m_FindMedian)
      {
        if(values.empty())
        {
          m_MedianArray->setValue(truePosition, 0.0f);
        }
        else
        {
          std::sort(values.begin(), values.end());
          if(values.size() % 2 == 1)
          {
            const usize halfElements = static_cast<usize>(std::floor(values.size() / 2.0f));
            m_MedianArray->setValue(truePosition, values[halfElements]);
          }
          else
          {
            const usize idxLow = (values.size() / 2) - 1;
            const usize idxHigh = values.size() / 2;
            m_MedianArray->setValue(truePosition, (values[idxLow] + values[idxHigh]) * 0.5f);
          }
        }
      }
      if(m_FindNumUniqueValues)
      {
        m_NumUniqueValuesArray->setValue(truePosition, static_cast<int32>(valuesSet.size()));
      }
    }
  }

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  bool m_FindMedian;
  bool m_FindNumUniqueValues;
  Float32Array* m_MedianArray;
  Int32Array* m_NumUniqueValuesArray;
  const BoolAbstractDataStore& m_Mask;
  const Int32AbstractDataStore& m_FeatureIds;
  const Int32AbstractDataStore& m_FeatureIdsMap;
  const AbstractDataStore<T>& m_Source;
  ComputeArrayStatistics* m_Filter = nullptr;
};

// -----------------------------------------------------------------------------
template <class ContainerType, typename T>
Result<> FindStatisticsImpl(const ContainerType& data, std::vector<IArray*>& arrays, const ComputeArrayStatisticsInputValues* inputValues)
{
  if(inputValues->FindLength)
  {
    auto* array0Ptr = dynamic_cast<DataArray<uint64>*>(arrays[0]);
    if(array0Ptr == nullptr)
    {
      return MakeErrorResult(-563501, fmt::format("Could not cast 'Length' array at path '{}' to UInt64Array.", inputValues->LengthArrayName.toString()));
    }
    const auto val = static_cast<uint64>(data.size());
    array0Ptr->initializeTuple(0, val);
  }

  // If we are finding the min or the max (or both) just combine that into a single call
  if(inputValues->FindMin || inputValues->FindMax)
  {
    const std::pair<T, T> minMaxValues = StatisticsCalculations::FindMinMax(data);
    if(inputValues->FindMin)
    {
      auto* array1Ptr = dynamic_cast<DataArray<T>*>(arrays[1]);
      if(array1Ptr == nullptr)
      {
        return MakeErrorResult(-563501, fmt::format("Could not cast 'Minimum' array at path '{}' to the expected type.", inputValues->MinimumArrayName.toString()));
      }
      array1Ptr->initializeTuple(0, minMaxValues.first);
    }
    if(inputValues->FindMax)
    {
      auto* array2Ptr = dynamic_cast<DataArray<T>*>(arrays[2]);
      if(array2Ptr == nullptr)
      {
        return MakeErrorResult(-563501, fmt::format("Could not cast 'Maximum' array at path '{}' to the expected type.", inputValues->MaximumArrayName.toString()));
      }
      array2Ptr->initializeTuple(0, minMaxValues.second);
    }
  }

  // Finding the mean depends on the summation.
  if(inputValues->FindSummation || inputValues->FindMean || inputValues->FindStdDeviation)
  {
    const std::pair<float, float> sumMeanValues = StatisticsCalculations::FindSumMean(data);
    if(inputValues->FindSummation)
    {
      auto* array6Ptr = dynamic_cast<Float32Array*>(arrays[7]);
      if(array6Ptr == nullptr)
      {
        return MakeErrorResult(-563501, fmt::format("Could not cast 'Summation' array at path '{}' to Float32Array.", inputValues->SummationArrayName.toString()));
      }
      array6Ptr->initializeTuple(0, sumMeanValues.first);
    }
    if(inputValues->FindMean)
    {
      auto* array3Ptr = dynamic_cast<Float32Array*>(arrays[3]);
      if(array3Ptr == nullptr)
      {
        return MakeErrorResult(-563501, fmt::format("Could not cast 'Mean' array at path '{}' to Float32Array.", inputValues->MeanArrayName.toString()));
      }
      array3Ptr->initializeTuple(0, sumMeanValues.second);
    }
    if(inputValues->FindStdDeviation)
    {
      auto* array5Ptr = dynamic_cast<Float32Array*>(arrays[6]);
      if(array5Ptr == nullptr)
      {
        return MakeErrorResult(-563501, fmt::format("Could not cast 'Standard Deviation' array at path '{}' to Float32Array.", inputValues->StdDeviationArrayName.toString()));
      }
      const float32 val = StatisticsCalculations::FindStdDeviation(data, sumMeanValues);
      array5Ptr->initializeTuple(0, val);
    }
  }

  if(inputValues->FindMedian)
  {
    auto* array4Ptr = dynamic_cast<Float32Array*>(arrays[4]);
    if(array4Ptr == nullptr)
    {
      throw std::invalid_argument(fmt::format("Could not cast 'Median' array at path '{}' to Float32Array.", inputValues->MedianArrayName.toString()));
    }
    const float32 val = StatisticsCalculations::findMedian(data);
    array4Ptr->initializeTuple(0, val);
  }

  if(inputValues->FindMode)
  {
    auto* array5Ptr = dynamic_cast<NeighborList<T>*>(arrays[5]);
    if(array5Ptr == nullptr)
    {
      throw std::invalid_argument(fmt::format("Could not cast 'Mode' array at path '{}' to the expected NeighborList type.", inputValues->ModeArrayName.toString()));
    }
    std::vector<T> modes = StatisticsCalculations::findModes(data);
    for(const auto& mode : modes)
    {
      array5Ptr->addEntry(0, mode);
    }
  }

  if(inputValues->FindNumUniqueValues)
  {
    auto* array8Ptr = dynamic_cast<DataArray<int32>*>(arrays[8]);
    if(array8Ptr == nullptr)
    {
      throw std::invalid_argument(fmt::format("Could not cast 'Number of Unique Values' array at path '{}' to Int32Array.", inputValues->NumUniqueValuesName.toString()));
    }
    const auto val = static_cast<int32>(StatisticsCalculations::findNumUniqueValues(data));
    array8Ptr->initializeTuple(0, val);
  }

  return {};
}

// -----------------------------------------------------------------------------
template <typename T>
Result<> InitializeArrays(DataStructure& dataStructure, const ComputeArrayStatisticsInputValues* inputValues)
{
  using InputDataArrayType = DataArray<T>;
  // Need to initialize the output data arrays
  if(inputValues->ComputeByIndex)
  {
    auto* arrayPtr = dataStructure.getDataAs<BoolArray>(inputValues->FeatureHasDataArrayName);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563502, fmt::format("Could not find or cast 'Feature Has Data' array at path '{}' to BoolArray.", inputValues->FeatureHasDataArrayName.toString()));
    }
    arrayPtr->fill(false);
  }
  if(inputValues->FindLength)
  {
    auto* arrayPtr = dataStructure.getDataAs<UInt64Array>(inputValues->LengthArrayName);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563503, fmt::format("Could not find or cast 'Length' array at path '{}' to UInt64Array.", inputValues->LengthArrayName.toString()));
    }
    arrayPtr->fill(0ULL);
  }
  if(inputValues->FindMin)
  {
    auto* arrayPtr = dataStructure.getDataAs<InputDataArrayType>(inputValues->MinimumArrayName);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563504, fmt::format("Could not find or cast 'Minimum' array at path '{}' to the expected type.", inputValues->MinimumArrayName.toString()));
    }
    arrayPtr->fill(static_cast<T>(std::numeric_limits<T>::max()));
  }
  if(inputValues->FindMax)
  {
    auto* arrayPtr = dataStructure.getDataAs<InputDataArrayType>(inputValues->MaximumArrayName);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563505, fmt::format("Could not find or cast 'Maximum' array at path '{}' to the expected type.", inputValues->MaximumArrayName.toString()));
    }
    arrayPtr->fill(static_cast<T>(std::numeric_limits<T>::min()));
  }
  if(inputValues->FindMean)
  {
    auto* arrayPtr = dataStructure.getDataAs<Float32Array>(inputValues->MeanArrayName);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563506, fmt::format("Could not find or cast 'Mean' array at path '{}' to Float32Array.", inputValues->MeanArrayName.toString()));
    }
    arrayPtr->fill(0.0F);
  }
  if(inputValues->FindMedian)
  {
    auto* arrayPtr = dataStructure.getDataAs<Float32Array>(inputValues->MedianArrayName);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563507, fmt::format("Could not find or cast 'Median' array at path '{}' to Float32Array.", inputValues->MedianArrayName.toString()));
    }
    arrayPtr->fill(0.0F);
  }
  if(inputValues->FindStdDeviation)
  {
    auto* arrayPtr = dataStructure.getDataAs<Float32Array>(inputValues->StdDeviationArrayName);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563509, fmt::format("Could not find or cast 'Standard Deviation' array at path '{}' to Float32Array.", inputValues->StdDeviationArrayName.toString()));
    }
    arrayPtr->fill(0.0F);
  }
  if(inputValues->FindSummation)
  {
    auto* arrayPtr = dataStructure.getDataAs<Float32Array>(inputValues->SummationArrayName);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563510, fmt::format("Could not find or cast 'Summation' array at path '{}' to Float32Array.", inputValues->SummationArrayName.toString()));
    }
    arrayPtr->fill(0.0F);
  }
  if(inputValues->FindNumUniqueValues)
  {
    auto* arrayPtr = dataStructure.getDataAs<Int32Array>(inputValues->NumUniqueValuesName);
    if(arrayPtr == nullptr)
    {
      return MakeErrorResult(-563513, fmt::format("Could not find or cast 'Number of Unique Values' array at path '{}' to Int32Array.", inputValues->NumUniqueValuesName.toString()));
    }
    arrayPtr->fill(-1);
  }
  // End Initialization

  return {};
}

// -----------------------------------------------------------------------------
struct ComputeArrayStatisticsFunctor
{
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const IDataArray& inputIDataArray, std::vector<IArray*>& arrays, const ComputeArrayStatisticsInputValues* inputValues)
  {
    std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare = nullptr;
    if(inputValues->UseMask)
    {
      try
      {
        maskCompare = MaskCompareUtilities::InstantiateMaskCompare(dataStructure, inputValues->MaskArrayPath);
      } catch(const std::out_of_range& exception)
      {
        // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
        // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
        const std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", inputValues->MaskArrayPath.toString());
        return MakeErrorResult(-563501, message);
      }
    }

    Result<> initializationResult = InitializeArrays<T>(dataStructure, inputValues);
    if(initializationResult.invalid())
    {
      return initializationResult;
    }

    const auto& inputArray = static_cast<const DataArray<T>&>(inputIDataArray);
    // this level checks whether computing by index or not and preps the calculations accordingly
    if(inputValues->UseMask)
    {
      // This section extracts out the data into a separate storage class. Note that
      // this could get real ugly for an out-of-core DataArray
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
      // compute the statistics for the entire array
      Result<> result = FindStatisticsImpl<std::vector<T>, T>(data, arrays, inputValues);
      if(result.invalid())
      {
        return result;
      }
    }
    else
    {
      // compute the statistics for the entire array
      Result<> result = FindStatisticsImpl<DataArray<T>, T>(inputArray, arrays, inputValues);
      if(result.invalid())
      {
        return result;
      }
    }

    // compute the standardized data based on whether computing by index or not
    if(inputValues->StandardizeData)
    {
      const auto& mean = dataStructure.getDataRefAs<Float32Array>(inputValues->MeanArrayName).getDataStoreRef();
      const auto& std = dataStructure.getDataRefAs<Float32Array>(inputValues->StdDeviationArrayName).getDataStoreRef();
      auto& standardized = dataStructure.getDataRefAs<Float32Array>(inputValues->StandardizedArrayName).getDataStoreRef();
      auto& data = inputArray.getDataStoreRef();

      const usize numTuples = data.getNumberOfTuples();

      for(usize i = 0; i < numTuples; i++)
      {
        if(!inputValues->UseMask || maskCompare->isTrue(i))
        {
          standardized.setValue(i, (static_cast<float32>(data[i]) - mean[0]) / std[0]);
        }
      }
    }
    return {};
  }
};

// -----------------------------------------------------------------------------
struct ComputeArrayStatisticsByFeatureFunctor
{
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const IDataArray* inputIDataArray, std::vector<IArray*>& arrays, usize numFeatures, const ComputeArrayStatisticsInputValues* inputValues,
                      const std::atomic_bool& shouldCancel, ComputeArrayStatistics* filter)
  {
    std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare = nullptr;
    if(inputValues->UseMask)
    {
      try
      {
        maskCompare = MaskCompareUtilities::InstantiateMaskCompare(dataStructure, inputValues->MaskArrayPath);
      } catch(const std::out_of_range& exception)
      {
        // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
        // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
        const std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", inputValues->MaskArrayPath.toString());
        return MakeErrorResult(-563508, message);
      }
    }

    Result<> initializationResult = InitializeArrays<T>(dataStructure, inputValues);
    if(initializationResult.invalid())
    {
      return initializationResult;
    }

    // this level preps and preforms the calculations accordingly
    const auto* inputArrayPtr = static_cast<const DataArray<T>*>(inputIDataArray);
    const auto* featureIdsPtr = dataStructure.getDataAs<Int32Array>(inputValues->FeatureIdsArrayPath);
    auto* lengthArrayPtr = dynamic_cast<DataArray<uint64>*>(arrays[0]);
    auto* minArrayPtr = dynamic_cast<DataArray<T>*>(arrays[1]);
    auto* maxArrayPtr = dynamic_cast<DataArray<T>*>(arrays[2]);
    auto* meanArrayPtr = dynamic_cast<Float32Array*>(arrays[3]);
    auto* modeArrayPtr = dynamic_cast<NeighborList<T>*>(arrays[5]);
    auto* stdDevArrayPtr = dynamic_cast<Float32Array*>(arrays[6]);
    auto* summationArrayPtr = dynamic_cast<Float32Array*>(arrays[7]);

    auto* featureHasDataPtr = dynamic_cast<BoolArray*>(arrays[9]);

    IParallelAlgorithm::AlgorithmArrays indexAlgArrays;
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
    StatisticsByFeatureImpl<T> classToExecute = StatisticsByFeatureImpl<T>(inputValues->FindLength, inputValues->FindMin, inputValues->FindMax, inputValues->FindMean, inputValues->FindMode,
                                                                           inputValues->FindStdDeviation, inputValues->FindSummation, maskCompare, featureIds, data, featureHasDataPtr, lengthArrayPtr,
                                                                           minArrayPtr, maxArrayPtr, meanArrayPtr, modeArrayPtr, stdDevArrayPtr, summationArrayPtr, shouldCancel, filter);
    if(CheckArraysInMemory(indexAlgArrays))
    {
      const tbb::simple_partitioner simplePartitioner;
      const usize grainSize = 500;
      tbb::blocked_range<usize> tbbRange(0, numFeatures, grainSize);
      tbb::parallel_for(tbbRange, std::move(classToExecute), simplePartitioner);
    }
    else
    {
      ParallelDataAlgorithm indexAlg;
      indexAlg.setRange(0, numFeatures);
      indexAlg.requireArraysInMemory(indexAlgArrays);
      indexAlg.execute(std::move(classToExecute));
    }

    if(inputValues->FindMedian || inputValues->FindNumUniqueValues)
    {
      filter->sendThreadSafeInfoMessage("Starting Median Calculation...");

      auto* medianArrayPtr = dynamic_cast<Float32Array*>(arrays[4]);
      auto* numUniqueValuesArrayPtr = dynamic_cast<Int32Array*>(arrays[8]);

      ParallelDataAlgorithm medianDataAlg;
      {
        // Scoped to prevent alg use of ptr array
        IParallelAlgorithm::AlgorithmArrays medianAlgArrays;
        medianAlgArrays.push_back(featureIdsPtr);
        medianAlgArrays.push_back(inputArrayPtr);
        medianAlgArrays.push_back(medianArrayPtr);
        medianAlgArrays.push_back(numUniqueValuesArrayPtr);
        medianAlgArrays.push_back(lengthArrayPtr);

        medianDataAlg.requireArraysInMemory(medianAlgArrays);
      }
      medianDataAlg.setRange(0, numFeatures);
      medianDataAlg.execute(
          MedianByFeatureImpl<T>(maskCompare, featureIds, data, inputValues->FindMedian, inputValues->FindNumUniqueValues, medianArrayPtr, numUniqueValuesArrayPtr, lengthArrayPtr, filter));
    }

    // compute the standardized data
    if(inputValues->StandardizeData)
    {
      const auto& mean = dataStructure.getDataRefAs<Float32Array>(inputValues->MeanArrayName).getDataStoreRef();
      const auto& std = dataStructure.getDataRefAs<Float32Array>(inputValues->StdDeviationArrayName).getDataStoreRef();
      auto& standardized = dataStructure.getDataRefAs<Float32Array>(inputValues->StandardizedArrayName).getDataStoreRef();

      const usize numTuples = data.getNumberOfTuples();
      for(usize i = 0; i < numTuples; i++)
      {
        if(!inputValues->UseMask || maskCompare->isTrue(i))
        {
          standardized.setValue(i, (static_cast<float32>(data[i]) - mean[featureIds.at(i)]) / std[featureIds.at(i)]);
        }
      }
    }
    return {};
  }

  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const IDataArray* inputIDataArray, std::vector<IArray*>& arrays, const std::pair<int32, int32>& range,
                      const ComputeArrayStatisticsInputValues* inputValues, const std::atomic_bool& shouldCancel, ComputeArrayStatistics* filter)
  {
    Result<> initializationResult = InitializeArrays<T>(dataStructure, inputValues);
    if(initializationResult.invalid())
    {
      return initializationResult;
    }

    // this level preps and preforms the calculations accordingly
    const auto* tempMaskPtr = dataStructure.getDataAs<BoolArray>(inputValues->TempMaskArrayPath); // this already accounts for previous mask
    const auto* featureIdsMapPtr = dataStructure.getDataAs<Int32Array>(inputValues->FeatureIdMapArrayPath);
    const auto* inputArrayPtr = static_cast<const DataArray<T>*>(inputIDataArray);
    const auto* featureIdsPtr = dataStructure.getDataAs<Int32Array>(inputValues->FeatureIdsArrayPath);
    auto* lengthArrayPtr = dynamic_cast<DataArray<uint64>*>(arrays[0]);
    auto* minArrayPtr = dynamic_cast<DataArray<T>*>(arrays[1]);
    auto* maxArrayPtr = dynamic_cast<DataArray<T>*>(arrays[2]);
    auto* meanArrayPtr = dynamic_cast<Float32Array*>(arrays[3]);
    auto* modeArrayPtr = dynamic_cast<NeighborList<T>*>(arrays[5]);
    auto* stdDevArrayPtr = dynamic_cast<Float32Array*>(arrays[6]);
    auto* summationArrayPtr = dynamic_cast<Float32Array*>(arrays[7]);

    auto* featureHasDataPtr = dynamic_cast<BoolArray*>(arrays[9]);

    IParallelAlgorithm::AlgorithmArrays indexAlgArrays;
    indexAlgArrays.push_back(tempMaskPtr);
    indexAlgArrays.push_back(featureIdsMapPtr);
    indexAlgArrays.push_back(featureIdsPtr);
    indexAlgArrays.push_back(inputArrayPtr);
    indexAlgArrays.push_back(featureHasDataPtr);
    indexAlgArrays.push_back(lengthArrayPtr);
    indexAlgArrays.push_back(minArrayPtr);
    indexAlgArrays.push_back(maxArrayPtr);
    indexAlgArrays.push_back(meanArrayPtr);
    indexAlgArrays.push_back(stdDevArrayPtr);
    indexAlgArrays.push_back(summationArrayPtr);

    const auto& featureIds = featureIdsPtr->getDataStoreRef();
    const auto& featureIdsMap = featureIdsMapPtr->getDataStoreRef();
    const auto& tempMask = tempMaskPtr->getDataStoreRef();
    const auto& data = inputArrayPtr->getDataStoreRef();
    StatisticsByFeatureRangeImpl<T> classToExecute = StatisticsByFeatureRangeImpl<T>(
        inputValues->FindLength, inputValues->FindMin, inputValues->FindMax, inputValues->FindMean, inputValues->FindMode, inputValues->FindStdDeviation, inputValues->FindSummation, featureIdsMap,
        tempMask, featureIds, data, featureHasDataPtr, lengthArrayPtr, minArrayPtr, maxArrayPtr, meanArrayPtr, modeArrayPtr, stdDevArrayPtr, summationArrayPtr, shouldCancel, filter);
    if(CheckArraysInMemory(indexAlgArrays))
    {
      const tbb::simple_partitioner simplePartitioner;
      const usize grainSize = 500;
      tbb::blocked_range<usize> tbbRange(range.first, range.second + 1, grainSize);
      tbb::parallel_for(tbbRange, std::move(classToExecute), simplePartitioner);
    }
    else
    {
      ParallelDataAlgorithm indexAlg;
      indexAlg.setRange(range.first, range.second + 1);
      indexAlg.requireArraysInMemory(indexAlgArrays);
      indexAlg.execute(std::move(classToExecute));
    }

    if(inputValues->FindMedian || inputValues->FindNumUniqueValues)
    {
      filter->sendThreadSafeInfoMessage("Starting Median Calculation...");

      auto* medianArrayPtr = dynamic_cast<Float32Array*>(arrays[4]);
      auto* numUniqueValuesArrayPtr = dynamic_cast<Int32Array*>(arrays[8]);

      ParallelDataAlgorithm medianDataAlg;
      {
        // Scoped to prevent alg use of ptr array
        IParallelAlgorithm::AlgorithmArrays medianAlgArrays;
        medianAlgArrays.push_back(featureIdsPtr);
        medianAlgArrays.push_back(inputArrayPtr);
        medianAlgArrays.push_back(medianArrayPtr);
        medianAlgArrays.push_back(numUniqueValuesArrayPtr);

        medianDataAlg.requireArraysInMemory(medianAlgArrays);
      }
      medianDataAlg.setRange(range.first, range.second + 1);
      medianDataAlg.execute(
          MedianByFeatureRangeImpl<T>(featureIdsMap, tempMask, featureIds, data, inputValues->FindMedian, inputValues->FindNumUniqueValues, medianArrayPtr, numUniqueValuesArrayPtr, filter));
    }

    // compute the standardized data based on whether computing by index or not
    if(inputValues->StandardizeData)
    {
      const auto& mean = dataStructure.getDataRefAs<Float32Array>(inputValues->MeanArrayName).getDataStoreRef();
      const auto& std = dataStructure.getDataRefAs<Float32Array>(inputValues->StdDeviationArrayName).getDataStoreRef();
      auto& standardized = dataStructure.getDataRefAs<Float32Array>(inputValues->StandardizedArrayName).getDataStoreRef();

      const usize numTuples = data.getNumberOfTuples();
      for(usize i = 0; i < numTuples; i++)
      {
        if(tempMask[i])
        {
          const usize truePosition = std::distance(featureIdsMap.begin(), std::find(featureIdsMap.begin(), featureIdsMap.end(), featureIds[i]));
          standardized.setValue(i, (static_cast<float32>(data[i]) - mean[truePosition]) / std[truePosition]);
        }
      }
    }
    return {};
  }
};
} // namespace

// -----------------------------------------------------------------------------
ComputeArrayStatistics::ComputeArrayStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeArrayStatisticsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
, m_Throttle(msgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeArrayStatistics::~ComputeArrayStatistics() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeArrayStatistics::sendThreadSafeInfoMessage(const std::string& message)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_MessageHandler.sendInfoMessage(message);
}

// -----------------------------------------------------------------------------
Result<> ComputeArrayStatistics::operator()()
{
  if(!m_InputValues->FindMin && !m_InputValues->FindMax && !m_InputValues->FindMean && !m_InputValues->FindMedian && !m_InputValues->FindMode && !m_InputValues->FindStdDeviation &&
     !m_InputValues->FindSummation && !m_InputValues->FindLength)
  {
    return {};
  }

  std::vector<IArray*> arrays(9, nullptr);

  if(m_InputValues->FindLength)
  {
    arrays[0] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->LengthArrayName);
  }
  if(m_InputValues->FindMin)
  {
    arrays[1] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MinimumArrayName);
  }
  if(m_InputValues->FindMax)
  {
    arrays[2] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaximumArrayName);
  }
  if(m_InputValues->FindMean)
  {
    arrays[3] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MeanArrayName);
  }
  if(m_InputValues->FindMedian)
  {
    arrays[4] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MedianArrayName);
  }
  if(m_InputValues->FindMode)
  {
    arrays[5] = m_DataStructure.getDataAs<INeighborList>(m_InputValues->ModeArrayName);
  }
  if(m_InputValues->FindStdDeviation)
  {
    arrays[6] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->StdDeviationArrayName);
  }
  if(m_InputValues->FindSummation)
  {
    arrays[7] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SummationArrayName);
  }
  if(m_InputValues->FindNumUniqueValues)
  {
    arrays[8] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->NumUniqueValuesName);
  }

  if(!m_InputValues->ComputeByIndex)
  {
    const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedArrayPath);

    // We must use ExecuteNeighborFunction because the Mode array is a NeighborList
    return ExecuteNeighborFunction(ComputeArrayStatisticsFunctor{}, inputArray.getDataType(), m_DataStructure, inputArray, arrays, m_InputValues);
  }

  arrays.resize(10);
  arrays[9] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->FeatureHasDataArrayName);

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  // Get max and min feature ids
  int32 trueMin = -1; // expected inclusive
  int32 trueMax = -1; // expected inclusive
  {
    const auto [min, max] = std::minmax_element(featureIds.cbegin(), featureIds.cend());
    trueMin = *min;
    trueMax = *max;
  }
  usize numFeatures = trueMax + 1;

  const FeatureIdRangeControls selection = static_cast<FeatureIdRangeControls>(m_InputValues->RangeType);

  // Unique Range of some sort if we made it here
  switch(selection)
  {
  case FeatureIdRangeControls::IgnoreZero: {
    // set number of features to the difference between max and 1 feature id
    trueMin = 1;
    break;
  }
  case FeatureIdRangeControls::ShrinkToFit: {
    // set number of features to the difference between max and min feature id
    break;
  }
  case FeatureIdRangeControls::CustomRange: {
    // set the bounds to difference between std::min(max, range.max) and std::max(min, range.min)
    trueMax = m_InputValues->Range.at(1) == -1 ? trueMax : std::min(trueMax, m_InputValues->Range.at(1));
    trueMin = std::max(trueMin, m_InputValues->Range.at(0));
    break;
  }
  case FeatureIdRangeControls::PaddedCustomRange: {
    // set number of features to the difference between max and min provided range
    trueMax = m_InputValues->Range.at(1) == -1 ? trueMax : m_InputValues->Range.at(1);
    trueMin = m_InputValues->Range.at(0);
    break;
  }
  case FeatureIdRangeControls::None: {
    auto* destAttrMatPtr = m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->DestinationAttributeMatrix);
    destAttrMatPtr->resizeTuples({numFeatures});
    const auto* inputArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SelectedArrayPath);

    // We must use ExecuteNeighborFunction because the Mode array is a NeighborList
    return ExecuteNeighborFunction(ComputeArrayStatisticsByFeatureFunctor{}, inputArray->getDataType(), m_DataStructure, inputArray, arrays, numFeatures, m_InputValues, m_ShouldCancel, this);
  }
  default: {
    return MakeErrorResult(-506670, fmt::format("Unknown feature id range controls option selected. Feature ID range was {} to {}.", trueMin, trueMax));
  }
  }
  if(trueMin > trueMax)
  {
    return MakeErrorResult(-506671, fmt::format("Range Error: Min value ({}) must be less than or equal to Max value ({})", trueMin, trueMax));
  }

  numFeatures = (trueMax - trueMin) + 1;

  // Temp Mask array created in preflight (for OoC compatibility)
  auto& tempMask = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->TempMaskArrayPath).getDataStoreRef();
  tempMask.fill(false);
  if(m_InputValues->UseMask)
  {
    std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare = nullptr;
    try
    {
      maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range& exception)
    {
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
      const std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
      return MakeErrorResult(-563501, message);
    }

    for(usize i = 0; i < featureIds.getNumberOfTuples(); i++)
    {
      // If using mask and mask value is false; mark false
      if(!maskCompare->isTrue(i))
      {
        continue;
      }

      // If value outside range; mark false
      const int32 featureId = featureIds[i];
      if(featureId > trueMax || featureId < trueMin)
      {
        continue;
      }

      tempMask[i] = true;
    }
  }
  else
  {
    for(usize i = 0; i < featureIds.getNumberOfTuples(); i++)
    {
      // If value outside range; mark false
      const int32 featureId = featureIds[i];
      if(featureId > trueMax || featureId < trueMin)
      {
        continue;
      }

      tempMask[i] = true;
    }
  }

  // Fill the feature id mapping array with std::iota from true min
  auto& featureIdMapping = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdMapArrayPath);
  featureIdMapping.resizeTuples(std::vector<usize>{numFeatures});
  std::iota(featureIdMapping.begin(), featureIdMapping.end(), trueMin);

  auto* destAttrMatPtr = m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->DestinationAttributeMatrix);
  destAttrMatPtr->resizeTuples({numFeatures});
  const auto* inputArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SelectedArrayPath);

  // We must use ExecuteNeighborFunction because the Mode array is a NeighborList
  return ExecuteNeighborFunction(ComputeArrayStatisticsByFeatureFunctor{}, inputArray->getDataType(), m_DataStructure, inputArray, arrays, std::make_pair(trueMin, trueMax), m_InputValues,
                                 m_ShouldCancel, this);
}
