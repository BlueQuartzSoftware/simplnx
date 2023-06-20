#include "MultiThresholdObjects.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Utilities/ArrayThreshold.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

namespace complex
{
namespace
{
constexpr int64 k_PathNotFoundError = -178;

class ThresholdFilterHelper
{
public:
  ThresholdFilterHelper(complex::ArrayThreshold::ComparisonType compType, complex::ArrayThreshold::ComparisonValue compValue, std::vector<bool>& output)
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
  std::vector<bool>& m_Output;

public:
  ThresholdFilterHelper(const ThresholdFilterHelper&) = delete;            // Copy Constructor Not Implemented
  ThresholdFilterHelper(ThresholdFilterHelper&&) = delete;                 // Move Constructor Not Implemented
  ThresholdFilterHelper& operator=(const ThresholdFilterHelper&) = delete; // Copy Assignment Not Implemented
  ThresholdFilterHelper& operator=(ThresholdFilterHelper&&) = delete;      // Move Assignment Not Implemented
};

struct ExecuteThresholdHelper
{
  template <typename Type>
  void operator()(ThresholdFilterHelper& helper, const IDataArray& iDataArray)
  {
    const auto& dataArray = dynamic_cast<const DataArray<Type>&>(iDataArray);
    helper.filterData<Type>(dataArray);
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
void InsertThreshold(usize numItems, BoolArray& currentArray, complex::IArrayThreshold::UnionOperator unionOperator, std::vector<bool>& newArrayPtr, bool inverse)
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
  auto& outputResultArray = dataStructure.getDataRefAs<BoolArray>(outputResultArrayPath);

  // Get the total number of tuples, create and initialize an array with FALSE to use for these results
  size_t totalTuples = outputResultArray.getNumberOfTuples();
  std::vector<bool> tempResultVector(totalTuples, false);

  complex::ArrayThreshold::ComparisonType compOperator = comparisonValue->getComparisonType();
  complex::ArrayThreshold::ComparisonValue compValue = comparisonValue->getComparisonValue();
  complex::IArrayThreshold::UnionOperator unionOperator = comparisonValue->getUnionOperator();

  DataPath inputDataArrayPath = comparisonValue->getArrayPath();

  ThresholdFilterHelper helper(compOperator, compValue, tempResultVector);

  const auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(inputDataArrayPath);

  ExecuteDataFunction(ExecuteThresholdHelper{}, iDataArray.getDataType(), helper, iDataArray);

  if(replaceInput)
  {
    if(inverse)
    {
      tempResultVector.flip();
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

  // Traditionally we would do a check to ensure we get a valid pointer, I'm forgoing that check because it
  // was essentially done in the preflight part.
  auto& outputResultArray = dataStructure.getDataRefAs<BoolArray>(outputResultArrayPath);

  // Get the total number of tuples, create and initialize an array with FALSE to use for these results
  size_t totalTuples = outputResultArray.getNumberOfTuples();
  std::vector<bool> tempResultVector(totalTuples, false);

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
      tempResultVector.flip();
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
  return {"Find Outliers", "Threshold", "Isolate", "Data Management"};
}

//------------------------------------------------------------------------------
Parameters MultiThresholdObjects::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(
      std::make_unique<ArrayThresholdsParameter>(k_ArrayThresholds_Key, "Data Thresholds", "DataArray thresholds to mask", ArrayThresholdSet{}, ArrayThresholdsParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CreatedDataPath_Key, "Mask Array", "DataPath to the created Mask Array", "Mask"));
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
  auto action = std::make_unique<CreateArrayAction>(DataType::boolean, dataArray->getIDataStore()->getTupleShape(), std::vector<usize>{1}, firstDataPath.getParent().createChildPath(maskArrayName),
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

  bool firstValueFound = false;
  DataPath maskArrayPath = (*thresholdsObject.getRequiredPaths().begin()).getParent().createChildPath(maskArrayName);
  int32_t err = 0;
  ArrayThresholdSet::CollectionType thresholdSet = thresholdsObject.getArrayThresholds();
  for(const std::shared_ptr<IArrayThreshold>& threshold : thresholdSet)
  {
    if(std::dynamic_pointer_cast<ArrayThresholdSet>(threshold))
    {
      std::shared_ptr<ArrayThresholdSet> comparisonSet = std::dynamic_pointer_cast<ArrayThresholdSet>(threshold);
      ThresholdSet(comparisonSet, dataStructure, maskArrayPath, err, !firstValueFound, thresholdsObject.isInverted());
      firstValueFound = true;
    }
    else if(std::dynamic_pointer_cast<ArrayThreshold>(threshold))
    {
      std::shared_ptr<ArrayThreshold> comparisonValue = std::dynamic_pointer_cast<ArrayThreshold>(threshold);
      ThresholdValue(comparisonValue, dataStructure, maskArrayPath, err, !firstValueFound, thresholdsObject.isInverted());
      firstValueFound = true;
    }
  }

  return {};
}
} // namespace complex
