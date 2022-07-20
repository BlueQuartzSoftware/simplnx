#include "FindArrayStatistics.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/Math/StatisticsCalculations.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

using namespace complex;

namespace
{
template <typename T>
class FindArrayStatisticsByIndexImpl
{
public:
  FindArrayStatisticsByIndexImpl(std::unordered_map<int32, std::list<T>>& featureDataMap, bool length, bool min, bool max, bool mean, bool median, bool stdDeviation, bool summation,
                                 std::vector<IDataArray*>& arrays, bool hist, float64 histmin, float64 histmax, bool histfullrange, int32 numBins)
  : m_FeatureDataMap(featureDataMap)
  , m_Length(length)
  , m_Min(min)
  , m_Max(max)
  , m_Mean(mean)
  , m_Median(median)
  , m_StdDeviation(stdDeviation)
  , m_Summation(summation)
  , m_Histogram(hist)
  , m_HistMin(histmin)
  , m_HistMax(histmax)
  , m_HistFullRange(histfullrange)
  , m_NumBins(numBins)
  , m_Arrays(arrays)
  {
  }

  virtual ~FindArrayStatisticsByIndexImpl() = default;

  void compute(usize start, usize end) const
  {
    auto* array0 = dynamic_cast<DataArray<uint64>*>(m_Arrays[0]);
    if(m_Length && array0 == nullptr)
    {
      throw std::invalid_argument("FindArrayStatisticsByIndexImpl::compute() could not dynamic_cast 'Length' array to needed type. Check input array selection.");
    }
    auto* array1 = dynamic_cast<DataArray<T>*>(m_Arrays[1]);
    if(m_Min && array1 == nullptr)
    {
      throw std::invalid_argument("FindArrayStatisticsByIndexImpl::compute() could not dynamic_cast 'Min' array to needed type. Check input array selection.");
    }
    auto* array2 = dynamic_cast<DataArray<T>*>(m_Arrays[2]);
    if(m_Max && array2 == nullptr)
    {
      throw std::invalid_argument("FindArrayStatisticsByIndexImpl::compute() could not dynamic_cast 'Max' array to needed type. Check input array selection.");
    }
    auto* array3 = dynamic_cast<Float32Array*>(m_Arrays[3]);
    if(m_Mean && array3 == nullptr)
    {
      throw std::invalid_argument("FindArrayStatisticsByIndexImpl::compute() could not dynamic_cast 'Mean' array to needed type. Check input array selection.");
    }
    auto* array4 = dynamic_cast<Float32Array*>(m_Arrays[4]);
    if(m_Median && array4 == nullptr)
    {
      throw std::invalid_argument("FindArrayStatisticsByIndexImpl::compute() could not dynamic_cast 'Median' array to needed type. Check input array selection.");
    }
    auto* array5 = dynamic_cast<Float32Array*>(m_Arrays[5]);
    if(m_StdDeviation && array5 == nullptr)
    {
      throw std::invalid_argument("FindArrayStatisticsByIndexImpl::compute() could not dynamic_cast 'StdDev' array to needed type. Check input array selection.");
    }
    auto* array6 = dynamic_cast<Float32Array*>(m_Arrays[6]);
    if(m_Summation && array6 == nullptr)
    {
      throw std::invalid_argument("FindArrayStatisticsByIndexImpl::compute() could not dynamic_cast 'Summation' array to needed type. Check input array selection.");
    }
    auto* array7 = dynamic_cast<Float32Array*>(m_Arrays[7]);
    if(m_Histogram && array7 == nullptr)
    {
      throw std::invalid_argument("FindArrayStatisticsByIndexImpl::compute() could not dynamic_cast 'Histogram' array to needed type. Check input array selection.");
    }

    for(usize i = start; i < end; i++)
    {
      if(m_Length)
      {
        uint64 val = static_cast<uint64>(m_FeatureDataMap[i].size());
        array0->initializeTuple(i, val);
      }
      if(m_Min)
      {
        T val = StaticicsCalculations::findMin(m_FeatureDataMap[i]);
        array1->initializeTuple(i, val);
      }
      if(m_Max)
      {
        T val = StaticicsCalculations::findMax(m_FeatureDataMap[i]);
        array2->initializeTuple(i, val);
      }
      if(m_Mean)
      {
        float32 val = StaticicsCalculations::findMean(m_FeatureDataMap[i]);
        array3->initializeTuple(i, val);
      }
      if(m_Median)
      {
        float32 val = StaticicsCalculations::findMedian(m_FeatureDataMap[i]);
        array4->initializeTuple(i, val);
      }
      if(m_StdDeviation)
      {
        float32 val = StaticicsCalculations::findStdDeviation(m_FeatureDataMap[i]);
        array5->initializeTuple(i, val);
      }
      if(m_Summation)
      {
        float32 val = StaticicsCalculations::findSummation(m_FeatureDataMap[i]);
        array6->initializeTuple(i, val);
      }
      if(m_Histogram)
      {
        auto* arr7DataStore = array7->getDataStore();
        if(arr7DataStore != nullptr)
        {
          std::vector<float32> vals = StaticicsCalculations::findHistogram(m_FeatureDataMap[i], m_HistMin, m_HistMax, m_HistFullRange, m_NumBins);
          arr7DataStore->setTuple(i, vals);
        }
      }
    }
  }

  void operator()(const ComplexRange& range) const
  {
    compute(range.min(), range.max());
  }

private:
  std::unordered_map<int32, std::list<T>>& m_FeatureDataMap;
  bool m_Length;
  bool m_Min;
  bool m_Max;
  bool m_Mean;
  bool m_Median;
  bool m_StdDeviation;
  bool m_Summation;
  bool m_Histogram;
  float64 m_HistMin;
  float64 m_HistMax;
  bool m_HistFullRange;
  int32 m_NumBins;
  std::vector<IDataArray*>& m_Arrays;
};

// -----------------------------------------------------------------------------
template <typename T>
void findStatisticsByIndexImpl(std::unordered_map<int32, std::list<T>>& featureDataMap, std::vector<IDataArray*>& arrays, const FindArrayStatisticsInputValues* inputValues, int32 numFeatures)
{
  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, numFeatures);
  dataAlg.execute(FindArrayStatisticsByIndexImpl<T>(featureDataMap, inputValues->FindLength, inputValues->FindMin, inputValues->FindMax, inputValues->FindMean, inputValues->FindMedian,
                                                    inputValues->FindStdDeviation, inputValues->FindSummation, arrays, inputValues->FindHistogram, inputValues->MinRange, inputValues->MaxRange,
                                                    inputValues->UseFullRange, inputValues->NumBins));
}

// -----------------------------------------------------------------------------
template <typename T>
void findStatisticsImpl(std::vector<T>& data, std::vector<IDataArray*>& arrays, const FindArrayStatisticsInputValues* inputValues)
{
  if(inputValues->FindLength)
  {
    auto* array0 = dynamic_cast<DataArray<uint64>*>(arrays[0]);
    if(array0 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'Length' array to needed type. Check input array selection.");
    }
    uint64 val = static_cast<uint64>(data.size());
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
    float32 val = StaticicsCalculations::findMean(data);
    array3->initializeTuple(0, val);
  }
  if(inputValues->FindMedian)
  {
    auto* array4 = dynamic_cast<Float32Array*>(arrays[4]);
    if(array4 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'Median' array to needed type. Check input array selection.");
    }
    float32 val = StaticicsCalculations::findMedian(data);
    array4->initializeTuple(0, val);
  }
  if(inputValues->FindStdDeviation)
  {
    auto* array5 = dynamic_cast<Float32Array*>(arrays[5]);
    if(array5 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'StdDev' array to needed type. Check input array selection.");
    }
    float32 val = StaticicsCalculations::findStdDeviation(data);
    array5->initializeTuple(0, val);
  }
  if(inputValues->FindSummation)
  {
    auto* array6 = dynamic_cast<Float32Array*>(arrays[6]);
    if(array6 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'Summation' array to needed type. Check input array selection.");
    }
    float32 val = StaticicsCalculations::findSummation(data);
    array6->initializeTuple(0, val);
  }
  if(inputValues->FindHistogram)
  {
    auto* array7 = dynamic_cast<Float32Array*>(arrays[7]);
    if(array7 == nullptr)
    {
      throw std::invalid_argument("findStatisticsImpl() could not dynamic_cast 'Histogram' array to needed type. Check input array selection.");
    }
    auto* arr7DataStore = array7->getDataStore();
    if(arr7DataStore != nullptr)
    {
      std::vector<float32> vals = StaticicsCalculations::findHistogram(data, inputValues->MinRange, inputValues->MaxRange, inputValues->UseFullRange, inputValues->NumBins);
      arr7DataStore->setTuple(0, vals);
    }
  }
}

// -----------------------------------------------------------------------------
template <typename T>
void findStatistics(const DataArray<T>& source, const Int32Array* featureIds, const std::unique_ptr<MaskCompare>& mask, const FindArrayStatisticsInputValues* inputValues,
                    std::vector<IDataArray*>& arrays, int32 numFeatures)
{
  usize numTuples = source.getNumberOfTuples();
  if(inputValues->ComputeByIndex)
  {
    std::unordered_map<int32, std::list<T>> featureValueMap;

    for(size_t i = 0; i < numTuples; i++)
    {
      if(inputValues->UseMask)
      {
        if(mask->isTrue(i))
        {
          featureValueMap[featureIds->at(i)].push_back(source[i]);
        }
      }
      else
      {
        featureValueMap[featureIds->at(i)].push_back(source[i]);
      }
    }

    findStatisticsByIndexImpl<T>(featureValueMap, arrays, inputValues, numFeatures);
  }
  else
  {
    std::vector<T> data;
    data.reserve(numTuples);
    for(size_t i = 0; i < numTuples; i++)
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

    findStatisticsImpl<T>(data, arrays, inputValues);
  }
}

// -----------------------------------------------------------------------------
template <typename T>
void standardizeDataByIndex(const DataArray<T>& data, bool useMask, const std::unique_ptr<MaskCompare>& mask, const Int32Array* featureIds, const Float32Array& mu, const Float32Array& sig,
                            Float32Array& standardized)
{
  size_t numTuples = data.getNumberOfTuples();
  for(size_t i = 0; i < numTuples; i++)
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
void standardizeData(const DataArray<T>& data, bool useMask, const std::unique_ptr<MaskCompare>& mask, const Float32Array& mu, const Float32Array& sig, Float32Array& standardized)
{
  size_t numTuples = data.getNumberOfTuples();

  for(size_t i = 0; i < numTuples; i++)
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
template <typename T>
Result<> FindArrayStatistics::findStats(const DataArray<T>& inputArray, std::vector<IDataArray*>& arrays, int32 numFeatures)
{
  Int32Array* featureIds = nullptr;
  if(m_InputValues->ComputeByIndex)
  {
    featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  }
  std::unique_ptr<MaskCompare> maskCompare = nullptr;
  if(m_InputValues->UseMask)
  {
    try
    {
      maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range& exception)
    {
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // somewhere else that is NOT going through the normal complex::IFilter API of Preflight and Execute
      std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
      return MakeErrorResult(-563502, message);
    }
  }
  findStatistics<T>(inputArray, featureIds, maskCompare, m_InputValues, arrays, numFeatures);

  if(m_InputValues->StandardizeData)
  {
    const Float32Array& mean = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MeanArrayName);
    const Float32Array& std = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->StdDeviationArrayName);
    Float32Array& standardized = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->StandardizedArrayName);

    if(m_InputValues->ComputeByIndex)
    {
      standardizeDataByIndex<T>(inputArray, m_InputValues->UseMask, maskCompare, featureIds, mean, std, standardized);
    }
    else
    {
      standardizeData<T>(inputArray, m_InputValues->UseMask, maskCompare, mean, std, standardized);
    }
  }
  return {};
}

// -----------------------------------------------------------------------------
Result<> FindArrayStatistics::operator()()
{
  if(!m_InputValues->FindHistogram && !m_InputValues->FindMin && !m_InputValues->FindMax && !m_InputValues->FindMean && !m_InputValues->FindMedian && !m_InputValues->FindStdDeviation &&
     !m_InputValues->FindSummation && !m_InputValues->FindLength)
  {
    return {};
  }

  int32 numFeatures = 0;

  // When we eventually get the Attribute Matrix class functionality back we will need to add these checks back in!
#if 0
  if(m_InputValues->ComputeByIndex)
  {
    const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
    const auto* destAttrMat = m_DataStructure.getData(m_InputValues->DestinationAttributeMatrix);
    numFeatures = static_cast<int32>(destAttrMat->getNumberOfTuples());
    bool mismatchedFeatures = false;
    int32 largestFeature = 0;
    size_t totalPoints = featureIds.getNumberOfTuples();

    for(size_t i = 0; i < totalPoints; i++)
    {
      if(featureIds[i] > largestFeature)
      {
        largestFeature = featureIds[i];
        if(largestFeature >= numFeatures)
        {
          mismatchedFeatures = true;
          break;
        }
      }
    }

    if(mismatchedFeatures)
    {
      return MakeErrorResult(-563500,
                             fmt::format("The number of objects in the selected Attribute Matrix destination ('{}') is larger than the largest Id in the Feature/Ensemble Ids array", numFeatures));
    }

    if(largestFeature != (numFeatures - 1))
    {
      return MakeErrorResult(-563501,
                             fmt::format("The number of objects in the selected Attribute Matrix destination ('{}') does not match the largest Id in the  Feature/Ensemble Ids array", numFeatures));
    }
  }
#endif

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

  const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedArrayPath);
  auto dataType = inputArray.getDataType();
  switch(dataType)
  {
  case DataType::int8: {
    return findStats<int8>(m_DataStructure.getDataRefAs<Int8Array>(m_InputValues->SelectedArrayPath), arrays, numFeatures);
  }
  case DataType::int16: {
    return findStats<int16>(m_DataStructure.getDataRefAs<Int16Array>(m_InputValues->SelectedArrayPath), arrays, numFeatures);
  }
  case DataType::int32: {
    return findStats<int32>(m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SelectedArrayPath), arrays, numFeatures);
  }
  case DataType::int64: {
    return findStats<int64>(m_DataStructure.getDataRefAs<Int64Array>(m_InputValues->SelectedArrayPath), arrays, numFeatures);
  }
  case DataType::uint8: {
    return findStats<uint8>(m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->SelectedArrayPath), arrays, numFeatures);
  }
  case DataType::uint16: {
    return findStats<uint16>(m_DataStructure.getDataRefAs<UInt16Array>(m_InputValues->SelectedArrayPath), arrays, numFeatures);
  }
  case DataType::uint32: {
    return findStats<uint32>(m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->SelectedArrayPath), arrays, numFeatures);
  }
  case DataType::uint64: {
    return findStats<uint64>(m_DataStructure.getDataRefAs<UInt64Array>(m_InputValues->SelectedArrayPath), arrays, numFeatures);
  }
  case DataType::float32: {
    return findStats<float32>(m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SelectedArrayPath), arrays, numFeatures);
  }
  case DataType::float64: {
    return findStats<float64>(m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->SelectedArrayPath), arrays, numFeatures);
  }
  case DataType::boolean: {
    return {};
  }
  }

  return {};
}
