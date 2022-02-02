#include "MultiThresholdObjects.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Utilities/ArrayThreshold.hpp"

namespace complex
{
namespace
{
constexpr StringLiteral k_ArrayThresholds_Key = "array_thresholds";
constexpr StringLiteral k_CreatedDataPath_Key = "created_data_path";

constexpr int64 k_PathNotFoundError = -178;

class ThresholdFilterHelper
{
public:
  ThresholdFilterHelper(complex::ArrayThreshold::ComparisonType compType, complex::ArrayThreshold::ComparisonValue compValue, std::vector<uint8_t>& output)
  : m_ComparisonOperator(compType)
  , m_ComparisonValue(compValue)
  , m_Output(output)
  {
  }

  ~ThresholdFilterHelper() = default;

  /**
   * @brief
   */
  template <typename T>
  void filterDataLessThan(const DataArray<T>& m_Input)
  {
    size_t m_NumValues = m_Input.getNumberOfTuples();
    T v = static_cast<T>(m_ComparisonValue);
    for(size_t i = 0; i < m_NumValues; ++i)
    {
      m_Output[i] = (m_Input[i] < v) ? 1 : 0;
    }
  }

  /**
   * @brief
   */
  template <typename T>
  void filterDataGreaterThan(const DataArray<T>& m_Input)
  {
    size_t m_NumValues = m_Input.getNumberOfTuples();
    T v = static_cast<T>(m_ComparisonValue);
    for(size_t i = 0; i < m_NumValues; ++i)
    {
      m_Output[i] = (m_Input[i] > v) ? 1 : 0;
    }
  }

  /**
   * @brief
   */
  template <typename T>
  void filterDataEqualTo(const DataArray<T>& m_Input)
  {
    size_t m_NumValues = m_Input.getNumberOfTuples();
    T v = static_cast<T>(m_ComparisonValue);
    for(size_t i = 0; i < m_NumValues; ++i)
    {
      m_Output[i] = (m_Input[i] == v) ? 1 : 0;
    }
  }

  /**
   * @brief
   */
  template <typename T>
  void filterDataNotEqualTo(const DataArray<T>& m_Input)
  {
    size_t m_NumValues = m_Input.getNumberOfTuples();
    T v = static_cast<T>(m_ComparisonValue);
    for(size_t i = 0; i < m_NumValues; ++i)
    {
      m_Output[i] = (m_Input[i] != v) ? 1 : 0;
    }
  }

  template <typename Type>
  void filterData(const DataArray<Type>& input)
  {
    if(m_ComparisonOperator == ArrayThreshold::ComparisonType::LessThan)
    {
      filterDataLessThan<Type>(input);
    }
    else if(m_ComparisonOperator == ArrayThreshold::ComparisonType::GreaterThan)
    {
      filterDataGreaterThan<Type>(input);
    }
    else if(m_ComparisonOperator == ArrayThreshold::ComparisonType::Operator_Equal)
    {
      filterDataEqualTo<Type>(input);
    }
    else if(m_ComparisonOperator == ArrayThreshold::ComparisonType::Operator_NotEqual)
    {
      filterDataNotEqualTo<Type>(input);
    }
    else
    {
      std::string errorMessage = fmt::format("MultiThresholdObjects Comparison Operator not understood: '{}'", static_cast<int>(m_ComparisonOperator));
      throw std::runtime_error(errorMessage);
    }
  }

  /**
   * @brief execute
   * @param input
   * @param output
   * @return
   */
  void execute(DataStructure& dataStructure, DataPath& inputArrayDataPath)
  {
    DataObject* dataObject = dataStructure.getData(inputArrayDataPath);

    if(const auto* dataArray = dynamic_cast<const DataArray<int8>*>(dataObject); dataArray != nullptr)
    {
      filterData<int8>(*dataArray);
    }
    else if(const auto* dataArray = dynamic_cast<const DataArray<int16>*>(dataObject); dataArray != nullptr)
    {
      filterData<int16>(*dataArray);
    }
    else if(const auto* dataArray = dynamic_cast<const DataArray<int32>*>(dataObject); dataArray != nullptr)
    {
      filterData<int32>(*dataArray);
    }
    else if(const auto* dataArray = dynamic_cast<const DataArray<int64>*>(dataObject); dataArray != nullptr)
    {
      filterData<int64>(*dataArray);
    }
    else if(const auto* dataArray = dynamic_cast<const DataArray<uint8>*>(dataObject); dataArray != nullptr)
    {
      filterData<uint8>(*dataArray);
    }
    else if(const auto* dataArray = dynamic_cast<const DataArray<uint16>*>(dataObject); dataArray != nullptr)
    {
      filterData<uint16>(*dataArray);
    }
    else if(const auto* dataArray = dynamic_cast<const DataArray<uint32>*>(dataObject); dataArray != nullptr)
    {
      filterData<uint32>(*dataArray);
    }
    else if(const auto* dataArray = dynamic_cast<const DataArray<uint64>*>(dataObject); dataArray != nullptr)
    {
      filterData<uint64>(*dataArray);
    }
    else if(const auto* dataArray = dynamic_cast<const DataArray<float32>*>(dataObject); dataArray != nullptr)
    {
      filterData<float32>(*dataArray);
    }
    else if(const auto* dataArray = dynamic_cast<const DataArray<float64>*>(dataObject); dataArray != nullptr)
    {
      filterData<float64>(*dataArray);
    }
    else if(const auto* dataArray = dynamic_cast<const DataArray<bool>*>(dataObject); dataArray != nullptr)
    {
      filterData<bool>(*dataArray);
    }
  }

private:
  complex::ArrayThreshold::ComparisonType m_ComparisonOperator;
  complex::ArrayThreshold::ComparisonValue m_ComparisonValue;
  std::vector<uint8_t>& m_Output;

public:
  ThresholdFilterHelper(const ThresholdFilterHelper&) = delete;            // Copy Constructor Not Implemented
  ThresholdFilterHelper(ThresholdFilterHelper&&) = delete;                 // Move Constructor Not Implemented
  ThresholdFilterHelper& operator=(const ThresholdFilterHelper&) = delete; // Copy Assignment Not Implemented
  ThresholdFilterHelper& operator=(ThresholdFilterHelper&&) = delete;      // Move Assignment Not Implemented
};

/**
 * @brief InsertThreshold
 * @param numItems
 * @param currentArrayPtr
 * @param unionOperator
 * @param newArrayPtr
 * @param inverse
 */
void InsertThreshold(int64_t numItems, UInt8Array& currentArray, complex::IArrayThreshold::UnionOperator unionOperator, std::vector<uint8_t>& newArrayPtr, bool inverse)
{
  for(int64_t i = 0; i < numItems; i++)
  {
    // invert the current comparison if necessary
    if(inverse)
    {
      (newArrayPtr[i] == 0 ? newArrayPtr[i] = 1 : newArrayPtr[i] = 0);
    }

    if(complex::IArrayThreshold::UnionOperator::Or == unionOperator)
    {
      (currentArray[i] != 0 || newArrayPtr[i] != 0) ? currentArray[i] = 1 : currentArray[i] = 0;
    }
    else if(currentArray[i] == 0 || newArrayPtr[i] == 0)
    {
      currentArray[i] = 0;
    }
  }
}

/**
 * @brief thresholdValue
 * @param comparisonValue
 * @param dataStructure
 * @param outputResultArrayPath
 * @param err
 * @param replaceInput
 * @param inverse
 */
void ThresholdValue(std::shared_ptr<ArrayThreshold>& comparisonValue, DataStructure& dataStructure, DataPath& outputResultArrayPath, int32_t& err, bool replaceInput, bool inverse)
{
  if(nullptr == comparisonValue)
  {
    err = -1;
    return;
  }
  // Traditionally we would do a check to ensure we get a valid pointer, I'm forgoing that check because it
  // was essentially done in the preflight part.
  UInt8Array& outputResultArray = dataStructure.getDataRefAs<UInt8Array>(outputResultArrayPath);

  // Get the total number of tuples, create and initialize an array with ZERO to use for these results
  size_t totalTuples = outputResultArray.getNumberOfTuples();
  std::vector<uint8_t> tempResultVector(totalTuples, 0xFF);

  complex::ArrayThreshold::ComparisonType compOperator = comparisonValue->getComparisonType();
  complex::ArrayThreshold::ComparisonValue compValue = comparisonValue->getComparisonValue();
  complex::IArrayThreshold::UnionOperator unionOperator = comparisonValue->getUnionOperator();

  DataPath inputDataArrayPath = comparisonValue->getArrayPath();

  ThresholdFilterHelper filter(compOperator, compValue, tempResultVector);

  filter.execute(dataStructure, inputDataArrayPath);

  if(replaceInput)
  {
    if(inverse)
    {
      std::for_each(tempResultVector.begin(), tempResultVector.end(), [](uint8_t& n) { (n == 0 ? n = 1 : n = 0); });
    }
    // copy the temp uint8 vector to the final uint8 result array
    for(size_t i = 0; i < totalTuples; i++)
    {
      outputResultArray[i] = tempResultVector[i];
    }
  }
  else
  {
    // insert into current threshold
    InsertThreshold(totalTuples, outputResultArray, unionOperator, tempResultVector, inverse);
  }
}

void ThresholdSet(std::shared_ptr<ArrayThresholdSet>& inputComparisonSet, DataStructure& dataStructure, DataPath& outputResultArrayPath, int32_t& err, bool replaceInput, bool inverse)
{
  if(nullptr == inputComparisonSet)
  {
    return;
  }

  //  if(inverse)
  //  {
  //    inverse = !comparisonSet->getInvertComparison();
  //  }
  //  else
  //  {
  //    inverse = comparisonSet->getInvertComparison();
  //  }

  // Traditionally we would do a check to ensure we get a valid pointer, I'm forgoing that check because it
  // was essentially done in the preflight part.
  UInt8Array& outputResultArray = dataStructure.getDataRefAs<UInt8Array>(outputResultArrayPath);

  // Get the total number of tuples, create and initialize an array with ZERO to use for these results
  size_t totalTuples = outputResultArray.getNumberOfTuples();
  std::vector<uint8_t> tempResultVector(totalTuples, 0xFF);

  bool firstValueFound = false;

  ArrayThresholdSet::CollectionType thresholds = inputComparisonSet->getArrayThresholds();
  for(const std::shared_ptr<IArrayThreshold>& threshold : thresholds)
  {
    if(std::dynamic_pointer_cast<ArrayThresholdSet>(threshold))
    {
      std::shared_ptr<ArrayThresholdSet> comparisonSet = std::dynamic_pointer_cast<ArrayThresholdSet>(threshold);
      ThresholdSet(comparisonSet, dataStructure, outputResultArrayPath, err, !firstValueFound, false);
      firstValueFound = true;
    }
    else if(std::dynamic_pointer_cast<ArrayThreshold>(threshold))
    {
      std::shared_ptr<ArrayThreshold> comparisonValue = std::dynamic_pointer_cast<ArrayThreshold>(threshold);
      ThresholdValue(comparisonValue, dataStructure, outputResultArrayPath, err, !firstValueFound, false);
      firstValueFound = true;
    }
  }

  if(replaceInput)
  {
    if(inverse)
    {
      std::for_each(tempResultVector.begin(), tempResultVector.end(), [](uint8_t& n) { (n == 0 ? n = 1 : n = 0); });
    }
    // copy the temp uint8 vector to the final uint8 result array
    for(size_t i = 0; i < totalTuples; i++)
    {
      outputResultArray[i] = tempResultVector[i];
    }
  }
  else
  {
    // insert into current threshold
    InsertThreshold(totalTuples, outputResultArray, inputComparisonSet->getUnionOperator(), tempResultVector, inverse);
  }
}

} // namespace

// -----------------------------------------------------------------------------
std::string MultiThresholdObjects::name() const
{
  return FilterTraits<MultiThresholdObjects>::name;
}

std::string MultiThresholdObjects::className() const
{
  return FilterTraits<MultiThresholdObjects>::className;
}

Uuid MultiThresholdObjects::uuid() const
{
  return FilterTraits<MultiThresholdObjects>::uuid;
}

std::string MultiThresholdObjects::humanName() const
{
  return "Multi-Threshold Objects";
}

Parameters MultiThresholdObjects::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<ArrayThresholdsParameter>(k_ArrayThresholds_Key, "Data Thresholds", "DataArray thresholds to mask", ArrayThresholdSet{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CreatedDataPath_Key, "Mask Array", "DataPath to the created Mask Array", DataPath{}));
  return params;
}

IFilter::UniquePointer MultiThresholdObjects::clone() const
{
  return std::make_unique<MultiThresholdObjects>();
}

std::vector<usize> getDims(const DataStructure& data, const std::set<DataPath>& dataPaths)
{
  if(dataPaths.empty())
  {
    return {0};
  }
  std::vector<usize> dims;

  DataPath firstDataPath = *(dataPaths.begin());
  const IDataArray* dataArray = data.getDataAs<IDataArray>(firstDataPath);
  size_t numTuples = dataArray->getNumberOfTuples();

  for(const auto& dataPath : dataPaths)
  {
    const IDataArray* currentDataArray = data.getDataAs<IDataArray>(dataPath);
    if(numTuples != dataArray->getNumberOfTuples())
    {
      return {};
    }
  }

  return {0};
}

IFilter::PreflightResult MultiThresholdObjects::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto thresholdsObject = args.value<ArrayThresholdSet>(k_ArrayThresholds_Key);
  auto maskArrayPath = args.value<DataPath>(k_CreatedDataPath_Key);

  auto thresholdPaths = thresholdsObject.getRequiredPaths();
  for(const auto& path : thresholdPaths)
  {
    if(data.getData(path) == nullptr)
    {
      auto errorMessage = fmt::format("Could not find DataArray at path {}.", path.toString());
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_PathNotFoundError, errorMessage}})};
    }
  }

  // Check for Scalar arrays
  for(const auto& dataPath : thresholdPaths)
  {
    const IDataArray* currentDataArray = data.getDataAs<IDataArray>(dataPath);
    if(currentDataArray->getNumberOfComponents() != 1)
    {
      auto errorMessage = fmt::format("Data Array is not a Scalar Data Array. Data Arrays must only have a single component. '{}:{}'", dataPath.toString(), currentDataArray->getNumberOfComponents());
      return {nonstd::make_unexpected(std::vector<Error>{Error{-4000, errorMessage}})};
    }
  }

  // Check that all arrays the number of tuples match
  DataPath firstDataPath = *(thresholdPaths.begin());
  const IDataArray* dataArray = data.getDataAs<IDataArray>(firstDataPath);
  size_t numTuples = dataArray->getNumberOfTuples();

  for(const auto& dataPath : thresholdPaths)
  {
    const IDataArray* currentDataArray = data.getDataAs<IDataArray>(dataPath);
    if(numTuples != currentDataArray->getNumberOfTuples())
    {
      auto errorMessage =
          fmt::format("Data Arrays do not have same equal number of tuples. '{}:{}' and '{}'", firstDataPath.toString(), numTuples, dataPath.toString(), currentDataArray->getNumberOfTuples());
      return {nonstd::make_unexpected(std::vector<Error>{Error{-4001, errorMessage}})};
    }
  }

  // Create the output boolean array
  auto action = std::make_unique<CreateArrayAction>(NumericType::uint8, dataArray->getIDataStore()->getTupleShape(), std::vector<usize>{1}, maskArrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

// -----------------------------------------------------------------------------
Result<> MultiThresholdObjects::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto thresholdsObject = args.value<ArrayThresholdSet>(k_ArrayThresholds_Key);
  auto maskArrayPath = args.value<DataPath>(k_CreatedDataPath_Key);

  bool firstValueFound = false;

  int32_t err = 0;
  ArrayThresholdSet::CollectionType thresholds = thresholdsObject.getArrayThresholds();

  for(const std::shared_ptr<IArrayThreshold>& threshold : thresholds)
  {
    if(std::dynamic_pointer_cast<ArrayThresholdSet>(threshold))
    {
      std::shared_ptr<ArrayThresholdSet> comparisonSet = std::dynamic_pointer_cast<ArrayThresholdSet>(threshold);
      ThresholdSet(comparisonSet, dataStructure, maskArrayPath, err, !firstValueFound, false);
      firstValueFound = true;
    }
    else if(std::dynamic_pointer_cast<ArrayThreshold>(threshold))
    {
      std::shared_ptr<ArrayThreshold> comparisonValue = std::dynamic_pointer_cast<ArrayThreshold>(threshold);
      ThresholdValue(comparisonValue, dataStructure, maskArrayPath, err, !firstValueFound, false);
      firstValueFound = true;
    }
  }

  // thresholdsObject.applyMaskValues(data, maskArrayPath);

  return {};
}
} // namespace complex
