#include "MultiThresholdObjects.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/DataTypeParameter.hpp"
#include "complex/Utilities/ArrayThreshold.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <algorithm>

namespace complex
{
namespace
{
constexpr int64 k_PathNotFoundError = -178;

template <typename U>
class ThresholdFilterHelper
{
public:
  ThresholdFilterHelper(complex::ArrayThreshold::ComparisonType compType, complex::ArrayThreshold::ComparisonValue compValue, std::vector<U>& output)
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
    T value = static_cast<T>(m_ComparisonValue);
    for(size_t i = 0; i < m_NumValues; ++i)
    {
      m_Output[i] = (m_Input[i] < value);
    }
  }

  /**
   * @brief
   */
  template <typename T>
  void filterDataGreaterThan(const DataArray<T>& m_Input)
  {
    size_t m_NumValues = m_Input.getNumberOfTuples();
    T value = static_cast<T>(m_ComparisonValue);
    for(size_t i = 0; i < m_NumValues; ++i)
    {
      m_Output[i] = (m_Input[i] > value);
    }
  }

  /**
   * @brief
   */
  template <typename T>
  void filterDataEqualTo(const DataArray<T>& m_Input)
  {
    size_t m_NumValues = m_Input.getNumberOfTuples();
    T value = static_cast<T>(m_ComparisonValue);
    for(size_t i = 0; i < m_NumValues; ++i)
    {
      m_Output[i] = (m_Input[i] == value);
    }
  }

  /**
   * @brief
   */
  template <typename T>
  void filterDataNotEqualTo(const DataArray<T>& m_Input)
  {
    size_t m_NumValues = m_Input.getNumberOfTuples();
    T value = static_cast<T>(m_ComparisonValue);
    for(size_t i = 0; i < m_NumValues; ++i)
    {
      m_Output[i] = (m_Input[i] != value);
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

private:
  complex::ArrayThreshold::ComparisonType m_ComparisonOperator;
  complex::ArrayThreshold::ComparisonValue m_ComparisonValue;
  std::vector<U>& m_Output;

public:
  ThresholdFilterHelper(const ThresholdFilterHelper&) = delete;            // Copy Constructor Not Implemented
  ThresholdFilterHelper(ThresholdFilterHelper&&) = delete;                 // Move Constructor Not Implemented
  ThresholdFilterHelper& operator=(const ThresholdFilterHelper&) = delete; // Copy Assignment Not Implemented
  ThresholdFilterHelper& operator=(ThresholdFilterHelper&&) = delete;      // Move Assignment Not Implemented
};

struct ExecuteThresholdHelper
{
  template <typename Type, typename MaskType>
  void operator()(ThresholdFilterHelper<MaskType>& helper, const IDataArray& iDataArray)
  {
    const auto& dataArray = dynamic_cast<const DataArray<Type>&>(iDataArray);
    helper.template filterData<Type>(dataArray);
  }
};

/**
 * @brief InsertThreshold
 * @param numItems
 * @param currentArrayPtr
 * @param unionOperator
 * @param newArrayPtr
 * @param inverse
 */
template <typename T>
void InsertThreshold(usize numItems, DataArray<T>& currentArray, complex::IArrayThreshold::UnionOperator unionOperator, std::vector<T>& newArrayPtr, bool inverse)
{
  for(usize i = 0; i < numItems; i++)
  {
    // invert the current comparison if necessary
    if(inverse)
    {
      newArrayPtr[i] = !newArrayPtr[i];
    }

    if(complex::IArrayThreshold::UnionOperator::Or == unionOperator)
    {
      currentArray[i] = currentArray[i] || newArrayPtr[i];
    }
    else if(!currentArray[i] || !newArrayPtr[i])
    {
      currentArray[i] = false;
    }
  }
}

template <typename T>
void ThresholdValue(std::shared_ptr<ArrayThreshold>& comparisonValue, DataStructure& dataStructure, DataPath& outputResultArrayPath, int32_t& err, bool replaceInput, bool inverse)
{
  if(nullptr == comparisonValue)
  {
    err = -1;
    return;
  }
  // Traditionally we would do a check to ensure we get a valid pointer, I'm forgoing that check because it
  // was essentially done in the preflight part.
  auto& outputResultArray = dataStructure.getDataRefAs<DataArray<T>>(outputResultArrayPath);

  // Get the total number of tuples, create and initialize an array with FALSE to use for these results
  size_t totalTuples = outputResultArray.getNumberOfTuples();
  std::vector<T> tempResultVector(totalTuples, false);

  complex::ArrayThreshold::ComparisonType compOperator = comparisonValue->getComparisonType();
  complex::ArrayThreshold::ComparisonValue compValue = comparisonValue->getComparisonValue();
  complex::IArrayThreshold::UnionOperator unionOperator = comparisonValue->getUnionOperator();

  DataPath inputDataArrayPath = comparisonValue->getArrayPath();

  ThresholdFilterHelper<T> helper(compOperator, compValue, tempResultVector);

  const auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(inputDataArrayPath);

  ExecuteDataFunction(ExecuteThresholdHelper{}, iDataArray.getDataType(), helper, iDataArray);

  if(replaceInput)
  {
    if(inverse)
    {
      std::reverse(tempResultVector.begin(), tempResultVector.end());
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
    InsertThreshold<T>(totalTuples, outputResultArray, unionOperator, tempResultVector, inverse);
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
void ThresholdValue(std::shared_ptr<ArrayThreshold>& comparisonValue, DataStructure& dataStructure, DataPath& outputResultArrayPath, DataType maskArrayType, int32_t& err, bool replaceInput,
                    bool inverse)
{
  switch(maskArrayType)
  {
  case DataType::int8:
    return ThresholdValue<int8>(comparisonValue, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::int16:
    return ThresholdValue<int16>(comparisonValue, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::int32:
    return ThresholdValue<int32>(comparisonValue, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::int64:
    return ThresholdValue<int64>(comparisonValue, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::uint8:
    return ThresholdValue<uint8>(comparisonValue, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::uint16:
    return ThresholdValue<uint16>(comparisonValue, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::uint32:
    return ThresholdValue<uint32>(comparisonValue, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::uint64:
    return ThresholdValue<uint64>(comparisonValue, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::float32:
    return ThresholdValue<float32>(comparisonValue, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::float64:
    return ThresholdValue<float64>(comparisonValue, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::boolean:
    [[fallthrough]];
  default:
    return ThresholdValue<bool>(comparisonValue, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  }
}

template <typename T>
void ThresholdSet(std::shared_ptr<ArrayThresholdSet>& inputComparisonSet, DataStructure& dataStructure, DataPath& outputResultArrayPath, int32_t& err, bool replaceInput, bool inverse)
{
  if(nullptr == inputComparisonSet)
  {
    return;
  }

  // Traditionally we would do a check to ensure we get a valid pointer, I'm forgoing that check because it
  // was essentially done in the preflight part.
  auto& outputResultArray = dataStructure.getDataRefAs<DataArray<T>>(outputResultArrayPath);

  // Get the total number of tuples, create and initialize an array with FALSE to use for these results
  size_t totalTuples = outputResultArray.getNumberOfTuples();
  std::vector<T> tempResultVector(totalTuples, false);

  T firstValueFound = 0;

  ArrayThresholdSet::CollectionType thresholds = inputComparisonSet->getArrayThresholds();
  for(const std::shared_ptr<IArrayThreshold>& threshold : thresholds)
  {
    if(std::dynamic_pointer_cast<ArrayThresholdSet>(threshold))
    {
      std::shared_ptr<ArrayThresholdSet> comparisonSet = std::dynamic_pointer_cast<ArrayThresholdSet>(threshold);
      ThresholdSet<T>(comparisonSet, dataStructure, outputResultArrayPath, err, !firstValueFound, false);
      firstValueFound = true;
    }
    else if(std::dynamic_pointer_cast<ArrayThreshold>(threshold))
    {
      std::shared_ptr<ArrayThreshold> comparisonValue = std::dynamic_pointer_cast<ArrayThreshold>(threshold);
      ThresholdValue<T>(comparisonValue, dataStructure, outputResultArrayPath, err, !firstValueFound, false);
      firstValueFound = true;
    }
  }

  if(replaceInput)
  {
    if(inverse)
    {
      std::reverse(tempResultVector.begin(), tempResultVector.end());
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
    InsertThreshold<T>(totalTuples, outputResultArray, inputComparisonSet->getUnionOperator(), tempResultVector, inverse);
  }
}

void ThresholdSet(std::shared_ptr<ArrayThresholdSet>& inputComparisonSet, DataStructure& dataStructure, DataPath& outputResultArrayPath, DataType maskArrayType, int32_t& err, bool replaceInput,
                  bool inverse)
{
  switch(maskArrayType)
  {
  case DataType::int8:
    return ThresholdSet<int8>(inputComparisonSet, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::int16:
    return ThresholdSet<int16>(inputComparisonSet, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::int32:
    return ThresholdSet<int32>(inputComparisonSet, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::int64:
    return ThresholdSet<int64>(inputComparisonSet, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::uint8:
    return ThresholdSet<uint8>(inputComparisonSet, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::uint16:
    return ThresholdSet<uint16>(inputComparisonSet, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::uint32:
    return ThresholdSet<uint32>(inputComparisonSet, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::uint64:
    return ThresholdSet<uint64>(inputComparisonSet, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::float32:
    return ThresholdSet<float32>(inputComparisonSet, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::float64:
    return ThresholdSet<float64>(inputComparisonSet, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  case DataType::boolean:
    [[fallthrough]];
  default:
    return ThresholdSet<bool>(inputComparisonSet, dataStructure, outputResultArrayPath, err, replaceInput, inverse);
  }
}

} // namespace

// -----------------------------------------------------------------------------
std::string MultiThresholdObjects::name() const
{
  return FilterTraits<MultiThresholdObjects>::name;
}

//------------------------------------------------------------------------------
std::string MultiThresholdObjects::className() const
{
  return FilterTraits<MultiThresholdObjects>::className;
}

//------------------------------------------------------------------------------
Uuid MultiThresholdObjects::uuid() const
{
  return FilterTraits<MultiThresholdObjects>::uuid;
}

//------------------------------------------------------------------------------
std::string MultiThresholdObjects::humanName() const
{
  return "Multi-Threshold Objects";
}

//------------------------------------------------------------------------------
std::vector<std::string> MultiThresholdObjects::defaultTags() const
{
  return {className(), "Find Outliers", "Threshold", "Isolate", "Data Management"};
}

//------------------------------------------------------------------------------
Parameters MultiThresholdObjects::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(
      std::make_unique<ArrayThresholdsParameter>(k_ArrayThresholds_Key, "Data Thresholds", "DataArray thresholds to mask", ArrayThresholdSet{}, ArrayThresholdsParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CreatedDataPath_Key, "Mask Array", "DataPath to the created Mask Array", "Mask"));
  params.insert(std::make_unique<DataTypeParameter>(k_CreatedMaskType_Key, "Mask Type", "DataType used for the created Mask Array", DataType::boolean));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MultiThresholdObjects::clone() const
{
  return std::make_unique<MultiThresholdObjects>();
}

// -----------------------------------------------------------------------------
IFilter::PreflightResult MultiThresholdObjects::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto thresholdsObject = args.value<ArrayThresholdSet>(k_ArrayThresholds_Key);
  auto maskArrayName = args.value<std::string>(k_CreatedDataPath_Key);
  auto maskArrayType = args.value<DataType>(k_CreatedMaskType_Key);

  auto thresholdPaths = thresholdsObject.getRequiredPaths();
  // If the paths are empty just return now.
  if(thresholdPaths.empty())
  {
    return MakePreflightErrorResult(-4000, "No data arrays were found for calculating threshold");
  }

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
    const auto* currentDataArray = data.getDataAs<IDataArray>(dataPath);
    if(currentDataArray != nullptr && currentDataArray->getNumberOfComponents() != 1)
    {
      auto errorMessage = fmt::format("Data Array is not a Scalar Data Array. Data Arrays must only have a single component. '{}:{}'", dataPath.toString(), currentDataArray->getNumberOfComponents());
      return {nonstd::make_unexpected(std::vector<Error>{Error{-4001, errorMessage}})};
    }
  }

  // Check that all arrays the number of tuples match
  DataPath firstDataPath = *(thresholdPaths.begin());
  const auto* dataArray = data.getDataAs<IDataArray>(firstDataPath);
  size_t numTuples = dataArray->getNumberOfTuples();

  for(const auto& dataPath : thresholdPaths)
  {
    const auto* currentDataArray = data.getDataAs<IDataArray>(dataPath);
    if(numTuples != currentDataArray->getNumberOfTuples())
    {
      auto errorMessage =
          fmt::format("Data Arrays do not have same equal number of tuples. '{}:{}' and '{}'", firstDataPath.toString(), numTuples, dataPath.toString(), currentDataArray->getNumberOfTuples());
      return {nonstd::make_unexpected(std::vector<Error>{Error{-4002, errorMessage}})};
    }
  }

  // Create the output boolean array
  auto action = std::make_unique<CreateArrayAction>(maskArrayType, dataArray->getIDataStore()->getTupleShape(), std::vector<usize>{1}, firstDataPath.getParent().createChildPath(maskArrayName),
                                                    dataArray->getDataFormat());

  OutputActions actions;
  actions.appendAction(std::move(action));

  return {std::move(actions)};
}

// -----------------------------------------------------------------------------
Result<> MultiThresholdObjects::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  auto thresholdsObject = args.value<ArrayThresholdSet>(k_ArrayThresholds_Key);
  auto maskArrayName = args.value<std::string>(k_CreatedDataPath_Key);
  auto maskArrayType = args.value<DataType>(k_CreatedMaskType_Key);

  bool firstValueFound = false;
  DataPath maskArrayPath = (*thresholdsObject.getRequiredPaths().begin()).getParent().createChildPath(maskArrayName);
  int32_t err = 0;
  ArrayThresholdSet::CollectionType thresholdSet = thresholdsObject.getArrayThresholds();
  for(const std::shared_ptr<IArrayThreshold>& threshold : thresholdSet)
  {
    if(std::dynamic_pointer_cast<ArrayThresholdSet>(threshold))
    {
      std::shared_ptr<ArrayThresholdSet> comparisonSet = std::dynamic_pointer_cast<ArrayThresholdSet>(threshold);
      ThresholdSet(comparisonSet, dataStructure, maskArrayPath, maskArrayType, err, !firstValueFound, thresholdsObject.isInverted());
      firstValueFound = true;
    }
    else if(std::dynamic_pointer_cast<ArrayThreshold>(threshold))
    {
      std::shared_ptr<ArrayThreshold> comparisonValue = std::dynamic_pointer_cast<ArrayThreshold>(threshold);
      ThresholdValue(comparisonValue, dataStructure, maskArrayPath, maskArrayType, err, !firstValueFound, thresholdsObject.isInverted());
      firstValueFound = true;
    }
  }

  return {};
}
} // namespace complex
