#include "FindArrayStatistics.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/Math/StatisticsCalculations.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include <unordered_map>

using namespace complex;

namespace
{
template <typename T>
class FindArrayStatisticsByIndexImpl
{
public:
  FindArrayStatisticsByIndexImpl(bool length, bool min, bool max, bool mean, bool stdDeviation, bool summation, bool hist, float64 histmin, float64 histmax, bool histfullrange, int32 numBins,
                                 const std::unique_ptr<MaskCompare>& mask, const Int32Array* featureIds, const DataArray<T>& source, UInt64Array* lengthArray, DataArray<T>* minArray,
                                 DataArray<T>* maxArray, Float32Array* meanArray, Float32Array* stdDevArray, Float32Array* summationArray, Float32Array* histArray, FindArrayStatistics* filter)
  : m_Length(length)
  , m_Min(min)
  , m_Max(max)
  , m_Mean(mean)
  , m_StdDeviation(stdDeviation)
  , m_Summation(summation)
  , m_Histogram(hist)
  , m_HistMin(histmin)
  , m_HistMax(histmax)
  , m_HistFullRange(histfullrange)
  , m_NumBins(numBins)
  , m_Mask(mask)
  , m_FeatureIds(featureIds)
  , m_Source(source)
  , m_LengthArray(lengthArray)
  , m_MinArray(minArray)
  , m_MaxArray(maxArray)
  , m_MeanArray(meanArray)
  , m_StdDevArray(stdDevArray)
  , m_SummationArray(summationArray)
  , m_HistArray(histArray)
  , m_Filter(filter)
  {
  }

  virtual ~FindArrayStatisticsByIndexImpl() = default;

  FindArrayStatisticsByIndexImpl(const FindArrayStatisticsByIndexImpl&) = default;           // Copy Constructor Not Implemented
  FindArrayStatisticsByIndexImpl(FindArrayStatisticsByIndexImpl&&) = delete;                 // Move Constructor Not Implemented
  FindArrayStatisticsByIndexImpl& operator=(const FindArrayStatisticsByIndexImpl&) = delete; // Copy Assignment Not Implemented
  FindArrayStatisticsByIndexImpl& operator=(FindArrayStatisticsByIndexImpl&&) = delete;      // Move Assignment Not Implemented

  void compute(usize start, usize end) const
  {

    std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    usize m_MilliDelay = 1000;

    const std::atomic_bool& shouldCancel = m_Filter->getCancel();
    const usize numTuples = m_FeatureIds->getNumberOfTuples(); // m_Messenger.getTotalElements();
    const usize numCurrentFeatures = end - start;

    std::vector<uint64> length(numCurrentFeatures, 0);
    std::vector<T> min(numCurrentFeatures, std::numeric_limits<T>::max());
    std::vector<T> max(numCurrentFeatures, std::numeric_limits<T>::min());
    std::vector<T> summation(numCurrentFeatures, 0);
    usize progressCount = 0;

    usize progressIncrement = numTuples / 100;

    for(usize i = 0; i < numTuples; ++i)
    {
      if(shouldCancel)
      {
        return;
      }
      if(m_Mask != nullptr && !m_Mask->isTrue(i))
      {
        continue;
      }
      for(usize j = 0; j < numCurrentFeatures; j++)
      {
        int32 featureId = (*m_FeatureIds)[i];
        if(featureId != static_cast<int32>(start + j))
        {
          continue;
        }

        ++length[j];

        if(m_Source[i] < min[j])
        {
          min[j] = m_Source[i];
        }

        if(m_Source[i] > max[j])
        {
          max[j] = m_Source[i];
        }

        summation[j] = summation[j] + m_Source[i];
      }
      progressCount++;
      now = std::chrono::steady_clock::now();
      if(progressCount > progressIncrement && std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > m_MilliDelay)
      {
        m_Filter->sendThreadSafeInfoMessage(fmt::format("Min/Max/Summation/Length Feature/Ensemble [{}-{}]: {:.2f}%", start, end, 100.0f * static_cast<float>(i) / static_cast<float>(numTuples)));
        progressCount = 0;
        m_InitialTime = std::chrono::steady_clock::now();
      }
    }

    AbstractDataStore<float32>* histDataStore = nullptr;
    if(m_HistArray != nullptr)
    {
      histDataStore = m_HistArray->getDataStore();
    }

    m_Filter->sendThreadSafeInfoMessage(fmt::format("Storing Results Feature/Ensemble Range [{}-{}]", start, end));
    progressIncrement = numCurrentFeatures / 100;
    progressCount = 0;
    for(usize j = start; j < end; j++)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize localFeatureIndex = j - start;
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
        m_SummationArray->initializeTuple(j, static_cast<float32>(summation[localFeatureIndex]));
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
          meanValue = static_cast<float32>(summation[localFeatureIndex]) / static_cast<float32>(length[localFeatureIndex]);
        }
      }

      if(m_Mean)
      {
        m_MeanArray->initializeTuple(j, meanValue);
      }

      if(m_Histogram && histDataStore != nullptr)
      {
        std::vector<float32> histogram(m_NumBins, 0);
        if(length[localFeatureIndex] > 0)
        {
          float32 histMin = 0.0f;
          float32 histMax = 0.0f;
          if(m_HistFullRange)
          {
            histMin = static_cast<float32>(min[localFeatureIndex]);
            histMax = static_cast<float32>(max[localFeatureIndex]);
          }
          else
          {
            histMin = m_HistMin;
            histMax = m_HistMax;
          }
          const float32 increment = (histMax - histMin) / (m_NumBins);
          if(std::fabs(increment) < 1E-10)
          {
            histogram = {static_cast<float32>(length[localFeatureIndex])};
          }
          else
          {
            for(usize i = 0; i < numTuples; ++i)
            {
              if(shouldCancel)
              {
                return;
              }
              if(m_Mask != nullptr && !m_Mask->isTrue(i))
              {
                continue;
              }
              if((*m_FeatureIds)[i] != static_cast<int32>(j))
              {
                continue;
              }
              const auto value = static_cast<float32>(m_Source[i]);
              const auto bin = static_cast<int32>((value - histMin) / increment); // find bin for this input array value
              if((bin >= 0) && (bin < m_NumBins))                                 // make certain bin is in range
              {
                ++histogram[bin]; // increment histogram element corresponding to this input array value
              }
              else if(value == histMax)
              {
                histogram[m_NumBins - 1]++;
              }
            } // end of numTuples loop
          }   // end of increment else
        }     // end of length if
        histDataStore->setTuple(j, histogram);
      } // end of m_Histogram if

      progressCount++;
      now = std::chrono::steady_clock::now();
      if(progressCount > progressIncrement && std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > m_MilliDelay)
      {
        m_Filter->sendThreadSafeInfoMessage(fmt::format("Storing data for feature/ensembles [{}-{}] {}/{}", start, end, start, end));
        progressCount = 0;
        m_InitialTime = std::chrono::steady_clock::now();
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
        if(shouldCancel)
        {
          return;
        }
        // Is the value in a mask and if so, is that mask TRUE
        if(m_Mask != nullptr && !m_Mask->isTrue(tupleIndex))
        {
          continue;
        }
        // Is the featureId within our range that we care about
        int32 featureId = (*m_FeatureIds)[tupleIndex];
        if(featureId < start || featureId >= end)
        {
          continue;
        }

        sumOfDiffs[featureId - start] += static_cast<float64>((m_Source[tupleIndex] - m_MeanArray->operator[](featureId)) * (m_Source[tupleIndex] - m_MeanArray->operator[](featureId)));

        progressCount++;
        now = std::chrono::steady_clock::now();
        if(progressCount > progressIncrement && std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > m_MilliDelay)
        {
          m_Filter->sendThreadSafeInfoMessage(fmt::format("StdDev Calculation Feature/Ensemble [{}-{}]: {:.2f}%", start, end, 100.0f * static_cast<float>(tupleIndex) / static_cast<float>(numTuples)));
          progressCount = 0;
          m_InitialTime = std::chrono::steady_clock::now();
        }
      }

      for(usize j = 0; j < numCurrentFeatures; j++)
      {
        // Set the value into the output array
        m_StdDevArray->operator[](j + start) = static_cast<float32>(std::sqrt(sumOfDiffs[j] / static_cast<float64>(m_LengthArray->operator[](j + start))));
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
  bool m_StdDeviation;
  bool m_Summation;
  bool m_Histogram;
  float64 m_HistMin;
  float64 m_HistMax;
  bool m_HistFullRange;
  int32 m_NumBins;
  const std::unique_ptr<MaskCompare>& m_Mask = nullptr;
  const Int32Array* m_FeatureIds = nullptr;
  const DataArray<T>& m_Source;
  UInt64Array* m_LengthArray = nullptr;
  DataArray<T>* m_MinArray = nullptr;
  DataArray<T>* m_MaxArray = nullptr;
  Float32Array* m_MeanArray = nullptr;
  Float32Array* m_StdDevArray = nullptr;
  Float32Array* m_SummationArray = nullptr;
  Float32Array* m_HistArray = nullptr;
  FindArrayStatistics* m_Filter = nullptr;
};

template <typename T>
class FindArrayMedianByIndexImpl
{
public:
  FindArrayMedianByIndexImpl(const std::unique_ptr<MaskCompare>& mask, const Int32Array* featureIds, const DataArray<T>& source, Float32Array* medianArray, DataArray<uint64>* lengthArray,
                             FindArrayStatistics* filter)
  : m_MedianArray(medianArray)
  , m_Mask(mask)
  , m_FeatureIds(featureIds)
  , m_Source(source)
  , m_LengthArray(lengthArray)
  , m_Filter(filter)
  {
  }

  virtual ~FindArrayMedianByIndexImpl() = default;

  void compute(usize start, usize end) const
  {
    m_Filter->sendThreadSafeInfoMessage(fmt::format("Starting Median Array Calculation: Feature/Ensemble [{}-{}]", start, end));

    usize numFeatureSources = end - start;
    // Create the arrays that will collect the values from the arrays. allocate them to the correct size based on the length array
    std::vector<std::vector<T>> featureSources(numFeatureSources);
    for(usize featureSourceIndex = 0; featureSourceIndex < numFeatureSources; featureSourceIndex++)
    {
      featureSources[featureSourceIndex].reserve(m_LengthArray->operator[](featureSourceIndex + start));
    }

    // std::cout << start << "    " << end << "  Starting Median Array Loop... " << std::endl;
    const usize numTuples = m_Source.getNumberOfTuples();
    const auto progressIncrement = numTuples / 100;
    usize progressCount = 0;
    for(usize tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
    {
      // Is the value in a mask and if so, is that mask TRUE
      if(m_Mask != nullptr && !m_Mask->isTrue(tupleIndex))
      {
        continue;
      }
      // Is the featureId within our range that we care about
      int32 featureId = (*m_FeatureIds)[tupleIndex];
      if(featureId < start || featureId >= end)
      {
        continue;
      }
      featureSources[featureId - start].push_back(m_Source[tupleIndex]);
    }

    for(usize featureSourceIndex = 0; featureSourceIndex < numFeatureSources; featureSourceIndex++)
    {
      const float32 val = StaticicsCalculations::findMedian(featureSources[featureSourceIndex]);
      m_MedianArray->operator[](featureSourceIndex + start) = val;
    }
  }

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  Float32Array* m_MedianArray;
  const std::unique_ptr<MaskCompare>& m_Mask = nullptr;
  const Int32Array* m_FeatureIds = nullptr;
  const DataArray<T>& m_Source;
  const DataArray<uint64>* m_LengthArray = nullptr;
  FindArrayStatistics* m_Filter = nullptr;
};

// -----------------------------------------------------------------------------
template <typename T>
void FindStatisticsImpl(std::vector<T>& data, std::vector<IDataArray*>& arrays, const FindArrayStatisticsInputValues* inputValues)
{
  if(inputValues->FindLength)
  {
    auto* array0 = dynamic_cast<DataArray<uint64>*>(arrays[0]);
    if(array0 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'Length' array to needed type. Check input array selection.");
    }
    const auto val = static_cast<uint64>(data.size());
    array0->initializeTuple(0, val);
  }
  if(inputValues->FindMin)
  {
    auto* array1 = dynamic_cast<DataArray<T>*>(arrays[1]);
    if(array1 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'Min' array to needed type. Check input array selection.");
    }
    T val = StaticicsCalculations::findMin(data);
    array1->initializeTuple(0, val);
  }
  if(inputValues->FindMax)
  {
    auto* array2 = dynamic_cast<DataArray<T>*>(arrays[2]);
    if(array2 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'Max' array to needed type. Check input array selection.");
    }
    T val = StaticicsCalculations::findMax(data);
    array2->initializeTuple(0, val);
  }
  if(inputValues->FindMean)
  {
    auto* array3 = dynamic_cast<Float32Array*>(arrays[3]);
    if(array3 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'Mean' array to needed type. Check input array selection.");
    }
    const float32 val = StaticicsCalculations::findMean(data);
    array3->initializeTuple(0, val);
  }
  if(inputValues->FindMedian)
  {
    auto* array4 = dynamic_cast<Float32Array*>(arrays[4]);
    if(array4 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'Median' array to needed type. Check input array selection.");
    }
    const float32 val = StaticicsCalculations::findMedian(data);
    array4->initializeTuple(0, val);
  }
  if(inputValues->FindStdDeviation)
  {
    auto* array5 = dynamic_cast<Float32Array*>(arrays[5]);
    if(array5 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'StdDev' array to needed type. Check input array selection.");
    }
    const float32 val = StaticicsCalculations::findStdDeviation(data);
    array5->initializeTuple(0, val);
  }
  if(inputValues->FindSummation)
  {
    auto* array6 = dynamic_cast<Float32Array*>(arrays[6]);
    if(array6 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'Summation' array to needed type. Check input array selection.");
    }
    const float32 val = StaticicsCalculations::findSummation(data);
    array6->initializeTuple(0, val);
  }
  if(inputValues->FindHistogram)
  {
    auto* array7 = dynamic_cast<Float32Array*>(arrays[7]);
    if(array7 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'Histogram' array to needed type. Check input array selection.");
    }

    if(auto* arr7DataStore = array7->getDataStore(); arr7DataStore != nullptr)
    {
      std::vector<float32> values = StaticicsCalculations::findHistogram(data, inputValues->MinRange, inputValues->MaxRange, inputValues->UseFullRange, inputValues->NumBins);
      arr7DataStore->setTuple(0, values);
    }
  }
}

// -----------------------------------------------------------------------------
template <typename T>
void FindStatistics(const DataArray<T>& source, const Int32Array* featureIds, const std::unique_ptr<MaskCompare>& mask, const FindArrayStatisticsInputValues* inputValues,
                    std::vector<IDataArray*>& arrays, usize numFeatures, FindArrayStatistics* filter)
{
  if(inputValues->ComputeByIndex)
  {
    auto* lengthArray = dynamic_cast<DataArray<uint64>*>(arrays[0]);
    auto* minArray = dynamic_cast<DataArray<T>*>(arrays[1]);
    auto* maxArray = dynamic_cast<DataArray<T>*>(arrays[2]);
    auto* meanArray = dynamic_cast<Float32Array*>(arrays[3]);
    auto* stdDevArray = dynamic_cast<Float32Array*>(arrays[5]);
    auto* summationArray = dynamic_cast<Float32Array*>(arrays[6]);
    auto* histArray = dynamic_cast<Float32Array*>(arrays[7]);

    // ThreadSafeMessenger messenger(messageHandler, " Computing Statistics by Feature/Ensemble....");
    // messenger.setTotalElements(numFeatures);
    // messenger.setProgressIncrement(numFeatures / 100);

    //    ParallelDataAlgorithm dataAlg;
    //    dataAlg.setRange(0, numFeatures);
    //    dataAlg.setParallelizationEnabled(true);
    //    dataAlg.setGrainSize(500);
    //    dataAlg.execute(FindArrayStatisticsByIndexImpl<T>(inputValues->FindLength, inputValues->FindMin, inputValues->FindMax, inputValues->FindMean, inputValues->FindStdDeviation,
    //                                                      inputValues->FindSummation, inputValues->FindHistogram, inputValues->MinRange, inputValues->MaxRange, inputValues->UseFullRange,
    //                                                      inputValues->NumBins, mask, featureIds, source, lengthArray, minArray, maxArray, meanArray, stdDevArray, summationArray, histArray,
    //                                                      filter));
#ifdef COMPLEX_ENABLE_MULTICORE
    tbb::simple_partitioner simplePartitioner;
    size_t grainSize = 500;
    tbb::blocked_range<size_t> tbbRange(0, numFeatures, grainSize);
    tbb::parallel_for(tbbRange,
                      FindArrayStatisticsByIndexImpl<T>(inputValues->FindLength, inputValues->FindMin, inputValues->FindMax, inputValues->FindMean, inputValues->FindStdDeviation,
                                                        inputValues->FindSummation, inputValues->FindHistogram, inputValues->MinRange, inputValues->MaxRange, inputValues->UseFullRange,
                                                        inputValues->NumBins, mask, featureIds, source, lengthArray, minArray, maxArray, meanArray, stdDevArray, summationArray, histArray, filter),
                      simplePartitioner);
#endif
    if(inputValues->FindMedian)
    {
      filter->sendThreadSafeInfoMessage("Starting Median Calculation..");

      auto* medianArrayPtr = dynamic_cast<Float32Array*>(arrays[4]);
      if(medianArrayPtr == nullptr)
      {
        throw std::invalid_argument("FindArrayMedianByIndexImpl could not dynamic_cast 'Median' array to a Float32 array type. Check input array selection.");
      }

      ParallelDataAlgorithm medianDataAlg;
      medianDataAlg.setParallelizationEnabled(false);
      medianDataAlg.setRange(0, numFeatures);
      medianDataAlg.execute(FindArrayMedianByIndexImpl<T>(mask, featureIds, source, medianArrayPtr, lengthArray, filter));
    }
  }
  else
  {
    usize numTuples = source.getNumberOfTuples();
    std::vector<T> data;
    data.reserve(numTuples);
    for(usize i = 0; i < numTuples; i++)
    {
      if(inputValues->UseMask)
      {
        if(mask->isTrue(i))
        {
          data.push_back(source[i]);
        }
      }
      else
      {
        data.push_back(source[i]);
      }
    }

    data.shrink_to_fit();

    // compute the statistics for the entire array
    FindStatisticsImpl<T>(data, arrays, inputValues);
  }
}

// -----------------------------------------------------------------------------
template <typename T>
void StandardizeDataByIndex(const DataArray<T>& data, bool useMask, const std::unique_ptr<MaskCompare>& mask, const Int32Array* featureIds, const Float32Array& mu, const Float32Array& sig,
                            Float32Array& standardized)
{
  const usize numTuples = data.getNumberOfTuples();
  for(usize i = 0; i < numTuples; i++)
  {
    if(useMask)
    {
      if(mask->isTrue(i))
      {
        standardized[i] = (static_cast<float32>(data[i]) - mu[featureIds->at(i)]) / sig[featureIds->at(i)];
      }
    }
    else
    {
      standardized[i] = (static_cast<float32>(data[i]) - mu[featureIds->at(i)]) / sig[featureIds->at(i)];
    }
  }
}

// -----------------------------------------------------------------------------
template <typename T>
void StandardizeData(const DataArray<T>& data, bool useMask, const std::unique_ptr<MaskCompare>& mask, const Float32Array& mu, const Float32Array& sig, Float32Array& standardized)
{
  const usize numTuples = data.getNumberOfTuples();

  for(usize i = 0; i < numTuples; i++)
  {
    if(useMask)
    {
      if(mask->isTrue(i))
      {
        standardized[i] = (static_cast<float32>(data[i]) - mu[0]) / sig[0];
      }
    }
    else
    {
      standardized[i] = (static_cast<float32>(data[i]) - mu[0]) / sig[0];
    }
  }
}

// -----------------------------------------------------------------------------
struct FindArrayStatisticsFunctor
{
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const IDataArray& inputIDataArray, std::vector<IDataArray*>& arrays, usize numFeatures, const FindArrayStatisticsInputValues* inputValues,
                      FindArrayStatistics* filter)
  {
    const auto& inputArray = static_cast<const DataArray<T>&>(inputIDataArray);
    Int32Array* featureIds = nullptr;
    if(inputValues->ComputeByIndex)
    {
      featureIds = dataStructure.getDataAs<Int32Array>(inputValues->FeatureIdsArrayPath);
    }
    std::unique_ptr<MaskCompare> maskCompare = nullptr;
    if(inputValues->UseMask)
    {
      try
      {
        maskCompare = InstantiateMaskCompare(dataStructure, inputValues->MaskArrayPath);
      } catch(const std::out_of_range& exception)
      {
        // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
        // somewhere else that is NOT going through the normal complex::IFilter API of Preflight and Execute
        const std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", inputValues->MaskArrayPath.toString());
        return MakeErrorResult(-563502, message);
      }
    }

    using InputDataArrayType = DataArray<T>;

    // Need to initialize the output data arrays
    if(inputValues->FindLength)
    {
      auto* arrayPtr = dataStructure.getDataAs<UInt64Array>(inputValues->LengthArrayName);
      if(arrayPtr == nullptr)
      {
        return MakeErrorResult(-563503, "FindArrayStatisticsFunctor could not dynamic_cast 'Length' array to needed type. Check input array selection.");
      }
      arrayPtr->fill(0ULL);
    }
    if(inputValues->FindMin)
    {
      auto* arrayPtr = dataStructure.getDataAs<InputDataArrayType>(inputValues->MinimumArrayName);
      if(arrayPtr == nullptr)
      {
        return MakeErrorResult(-563504, "FindArrayStatisticsFunctor could not dynamic_cast 'Min' array to needed type. Check input array selection.");
      }
      arrayPtr->fill(static_cast<T>(std::numeric_limits<T>::max()));
    }
    if(inputValues->FindMax)
    {
      auto* arrayPtr = dataStructure.getDataAs<InputDataArrayType>(inputValues->MaximumArrayName);
      if(arrayPtr == nullptr)
      {
        return MakeErrorResult(-563505, "FindArrayStatisticsFunctor could not dynamic_cast 'Max' array to needed type. Check input array selection.");
      }
      arrayPtr->fill(static_cast<T>(std::numeric_limits<T>::min()));
    }
    if(inputValues->FindMean)
    {
      auto* arrayPtr = dataStructure.getDataAs<Float32Array>(inputValues->MeanArrayName);
      if(arrayPtr == nullptr)
      {
        return MakeErrorResult(-563506, "FindArrayStatisticsFunctor could not dynamic_cast 'Mean' array to needed type. Check input array selection.");
      }
      arrayPtr->fill(0.0F);
    }
    if(inputValues->FindMedian)
    {
      auto* arrayPtr = dataStructure.getDataAs<Float32Array>(inputValues->MedianArrayName);
      if(arrayPtr == nullptr)
      {
        return MakeErrorResult(-563507, "FindArrayStatisticsFunctor could not dynamic_cast 'Median' array to needed type. Check input array selection.");
      }
      arrayPtr->fill(0.0F);
    }
    if(inputValues->FindStdDeviation)
    {
      auto* arrayPtr = dataStructure.getDataAs<Float32Array>(inputValues->StdDeviationArrayName);
      if(arrayPtr == nullptr)
      {
        return MakeErrorResult(-563508, "FindArrayStatisticsFunctor could not dynamic_cast 'Standard Deviation' array to needed type. Check input array selection.");
      }
      arrayPtr->fill(0.0F);
    }
    if(inputValues->FindSummation)
    {
      auto* arrayPtr = dataStructure.getDataAs<Float32Array>(inputValues->SummationArrayName);
      if(arrayPtr == nullptr)
      {
        return MakeErrorResult(-563509, "FindArrayStatisticsFunctor could not dynamic_cast 'Summation' array to needed type. Check input array selection.");
      }
      arrayPtr->fill(0.0F);
    }
    if(inputValues->FindHistogram)
    {
      auto* arrayPtr = dataStructure.getDataAs<Float32Array>(inputValues->HistogramArrayName);
      if(arrayPtr == nullptr)
      {
        return MakeErrorResult(-563510, "FindArrayStatisticsFunctor could not dynamic_cast 'Histogram' array to needed type. Check input array selection.");
      }
      arrayPtr->fill(0.0F);
    }
    // End Initialization

    // this level checks whether computing by index or not and preps the calculations accordingly
    FindStatistics<T>(inputArray, featureIds, maskCompare, inputValues, arrays, numFeatures, filter);

    // compute the standardized data based on whether computing by index or not
    if(inputValues->StandardizeData)
    {
      const auto& mean = dataStructure.getDataRefAs<Float32Array>(inputValues->MeanArrayName);
      const auto& std = dataStructure.getDataRefAs<Float32Array>(inputValues->StdDeviationArrayName);
      auto& standardized = dataStructure.getDataRefAs<Float32Array>(inputValues->StandardizedArrayName);

      if(inputValues->ComputeByIndex)
      {
        StandardizeDataByIndex<T>(inputArray, inputValues->UseMask, maskCompare, featureIds, mean, std, standardized);
      }
      else
      {
        StandardizeData<T>(inputArray, inputValues->UseMask, maskCompare, mean, std, standardized);
      }
    }
    return {};
  }
};
} // namespace

// -----------------------------------------------------------------------------
FindArrayStatistics::FindArrayStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindArrayStatisticsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindArrayStatistics::~FindArrayStatistics() noexcept = default;

// -----------------------------------------------------------------------------
Result<> FindArrayStatistics::operator()()
{
  if(!m_InputValues->FindHistogram && !m_InputValues->FindMin && !m_InputValues->FindMax && !m_InputValues->FindMean && !m_InputValues->FindMedian && !m_InputValues->FindStdDeviation &&
     !m_InputValues->FindSummation && !m_InputValues->FindLength)
  {
    return {};
  }

  std::vector<IDataArray*> arrays(8, nullptr);

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
  if(m_InputValues->FindStdDeviation)
  {
    arrays[5] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->StdDeviationArrayName);
  }
  if(m_InputValues->FindSummation)
  {
    arrays[6] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SummationArrayName);
  }
  if(m_InputValues->FindHistogram)
  {
    arrays[7] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->HistogramArrayName);
  }

  usize numFeatures = 0;
  if(m_InputValues->ComputeByIndex)
  {
    const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
    numFeatures = findNumFeatures(featureIds);

    auto* destAttrMat = m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->DestinationAttributeMatrix);
    destAttrMat->setShape({numFeatures});

    for(const auto& array : arrays)
    {
      if(array != nullptr)
      {
        array->reshapeTuples({numFeatures});
      }
    }
  }

  const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedArrayPath);

  ExecuteDataFunction(FindArrayStatisticsFunctor{}, inputArray.getDataType(), m_DataStructure, inputArray, arrays, numFeatures, m_InputValues, this);

  return {};
}

// -----------------------------------------------------------------------------
const std::atomic_bool& FindArrayStatistics::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void FindArrayStatistics::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressCounter += counter;
  auto progressInt = static_cast<size_t>((static_cast<float32>(m_ProgressCounter) / static_cast<float32>(m_TotalElements)) * 100.0f);

  size_t progIncrement = m_TotalElements / 100;

  if(m_ProgressCounter > 1 && m_LastProgressInt != progressInt)
  {
    std::string ss = fmt::format("Finding Distances || {}% Completed", progressInt);
    m_MessageHandler(IFilter::Message::Type::Info, ss);
  }

  m_LastProgressInt = progressInt;
}

// -----------------------------------------------------------------------------
void FindArrayStatistics::sendThreadSafeInfoMessage(const std::string& message)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  auto now = std::chrono::steady_clock::now();
  if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > m_MilliDelay)
  {
    m_MessageHandler(IFilter::Message{IFilter::Message::Type::Info, message});
    m_InitialTime = std::chrono::steady_clock::now();
  }
}

// -----------------------------------------------------------------------------
usize FindArrayStatistics::findNumFeatures(const Int32Array& featureIds) const
{
  m_MessageHandler(IFilter::Message{IFilter::Message::Type::Info, "Finding Max FeatureId..."});
  usize numFeatures = 0;
  const usize totalPoints = featureIds.getNumberOfTuples();
  for(usize i = 0; i < totalPoints; i++)
  {
    if(featureIds[i] > numFeatures)
    {
      numFeatures = featureIds[i];
    }
  }
  return numFeatures + 1;
}
