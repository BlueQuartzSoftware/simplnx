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
                                 DataArray<T>* maxArray, Float32Array* meanArray, Float32Array* stdDevArray, Float32Array* summationArray, Float32Array* histArray)
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
  {
  }

  virtual ~FindArrayStatisticsByIndexImpl() = default;

  void compute(usize start, usize end) const
  {
    const usize numTuples = m_Source.getNumberOfTuples();
    const usize numCurrentFeatures = end - start;

    std::vector<uint64> length(numCurrentFeatures, 0);
    std::vector<T> min(numCurrentFeatures, std::numeric_limits<T>::max());
    std::vector<T> max(numCurrentFeatures, std::numeric_limits<T>::min());
    std::vector<T> summation(numCurrentFeatures, 0);
    for(usize i = 0; i < numTuples; ++i)
    {
      if(m_Mask != nullptr && !m_Mask->isTrue(i))
      {
        continue;
      }
      for(usize j = 0; j < numCurrentFeatures; j++)
      {
        if((*m_FeatureIds)[i] != static_cast<int32>(start + j))
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
    }

    AbstractDataStore<float32>* histDataStore = nullptr;
    if(m_HistArray != nullptr)
    {
      histDataStore = m_HistArray->getDataStore();
    }
    for(usize j = start; j < end; j++)
    {
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
      if(m_StdDeviation)
      {
        float32 stdDevVal = 0.0f;
        if(length[localFeatureIndex] > 0)
        {
          if constexpr(std::is_same_v<T, bool>)
          {
            stdDevVal = meanValue;
          }
          else
          {
            std::vector<float64> difference;
            difference.reserve(numTuples);
            for(usize i = 0; i < numTuples; ++i)
            {
              if(m_Mask != nullptr && !m_Mask->isTrue(i))
              {
                continue;
              }
              if((*m_FeatureIds)[i] == static_cast<int32>(j))
              {
                difference.push_back(static_cast<float64>(m_Source[i] - meanValue));
              }
            }
            const float64 squaredSum = std::inner_product(std::cbegin(difference), std::cend(difference), std::cbegin(difference), 0.0);
            stdDevVal = static_cast<float32>(std::sqrt(squaredSum / static_cast<float64>(difference.size())));
          }
        }
        m_StdDevArray->initializeTuple(j, stdDevVal);
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
    }   // end of outermost for loop
  }     // end of compute

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
};

template <typename T>
class FindArrayMedianByIndexImpl
{
public:
  FindArrayMedianByIndexImpl(const std::unique_ptr<MaskCompare>& mask, const Int32Array* featureIds, const DataArray<T>& source, Float32Array* medianArray)
  : m_MedianArray(medianArray)
  , m_Mask(mask)
  , m_FeatureIds(featureIds)
  , m_Source(source)
  {
  }

  virtual ~FindArrayMedianByIndexImpl() = default;

  void compute(usize start, usize end) const
  {
    const usize numTuples = m_Source.getNumberOfTuples();
    for(usize j = start; j < end; j++)
    {
      std::vector<T> featureSource;
      usize reserveSize = 10; // not sure what to put here so making a guess
      featureSource.reserve(reserveSize);
      usize numPushBacks = 0;
      for(usize i = 0; i < numTuples; ++i)
      {
        if(m_Mask != nullptr && !m_Mask->isTrue(i))
        {
          continue;
        }
        if((*m_FeatureIds)[i] != static_cast<int32>(j))
        {
          continue;
        }
        if(numPushBacks >= reserveSize)
        {
          reserveSize *= 10;
          featureSource.reserve(reserveSize);
        }
        featureSource.push_back(m_Source[i]);
        ++numPushBacks;
      }
      const float32 val = StaticicsCalculations::findMedian(featureSource);
      m_MedianArray->initializeTuple(j, val);
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
                    std::vector<IDataArray*>& arrays, usize numFeatures)
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

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, numFeatures);
    dataAlg.execute(FindArrayStatisticsByIndexImpl<T>(inputValues->FindLength, inputValues->FindMin, inputValues->FindMax, inputValues->FindMean, inputValues->FindStdDeviation,
                                                      inputValues->FindSummation, inputValues->FindHistogram, inputValues->MinRange, inputValues->MaxRange, inputValues->UseFullRange,
                                                      inputValues->NumBins, mask, featureIds, source, lengthArray, minArray, maxArray, meanArray, stdDevArray, summationArray, histArray));
    if(inputValues->FindMedian)
    {
      auto* medianArray = dynamic_cast<Float32Array*>(arrays[4]);
      if(medianArray == nullptr)
      {
        throw std::invalid_argument("FindArrayMedianByIndexImpl could not dynamic_cast 'Median' array to needed type. Check input array selection.");
      }

      ParallelDataAlgorithm medianDataAlg;
      medianDataAlg.setRange(0, numFeatures);
      medianDataAlg.execute(FindArrayMedianByIndexImpl<T>(mask, featureIds, source, medianArray));
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
  Result<> operator()(DataStructure& dataStructure, const IDataArray& inputIDataArray, std::vector<IDataArray*>& arrays, usize numFeatures, const FindArrayStatisticsInputValues* inputValues)
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
    FindStatistics<T>(inputArray, featureIds, maskCompare, inputValues, arrays, numFeatures);

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
const std::atomic_bool& FindArrayStatistics::getCancel()
{
  return m_ShouldCancel;
}

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
    numFeatures = FindNumFeatures(featureIds);

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

  ExecuteDataFunction(FindArrayStatisticsFunctor{}, inputArray.getDataType(), m_DataStructure, inputArray, arrays, numFeatures, m_InputValues);

  return {};
}

// -----------------------------------------------------------------------------
usize FindArrayStatistics::FindNumFeatures(const Int32Array& featureIds)
{
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
