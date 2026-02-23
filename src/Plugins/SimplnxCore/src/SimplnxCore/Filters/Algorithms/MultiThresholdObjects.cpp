#include "MultiThresholdObjects.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ArrayThreshold.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>

using namespace nx::core;

namespace
{
template <class U>
class ThresholdFilterHelper
{
public:
  ThresholdFilterHelper(ArrayThreshold::ComparisonType compType, ArrayThreshold::ComparisonValue compValue, usize componentIndex, std::vector<U>& output)
  : m_ComparisonOperator(compType)
  , m_ComparisonValue(compValue)
  , m_ComponentIndex(componentIndex)
  , m_Output(output)
  {
  }

  template <class CompT, class T>
  void filterDataWithComparision(const AbstractDataStore<T>& m_Input, T trueValue, T falseValue)
  {
    size_t numTuples = m_Input.getNumberOfTuples();
    T value = static_cast<T>(m_ComparisonValue);
    for(size_t tupleIndex = 0; tupleIndex < numTuples; ++tupleIndex)
    {
      T inputValue = m_Input.getComponentValue(tupleIndex, m_ComponentIndex);
      T outputValue = CompT{}(inputValue, value) ? trueValue : falseValue;
      m_Output[tupleIndex] = outputValue;
    }
  }

  template <class T>
  void filterData(const AbstractDataStore<T>& input, T trueValue, T falseValue)
  {
    if(m_ComparisonOperator == ArrayThreshold::ComparisonType::LessThan)
    {
      filterDataWithComparision<std::less<>, T>(input, trueValue, falseValue);
    }
    else if(m_ComparisonOperator == ArrayThreshold::ComparisonType::GreaterThan)
    {
      filterDataWithComparision<std::greater<>, T>(input, trueValue, falseValue);
    }
    else if(m_ComparisonOperator == ArrayThreshold::ComparisonType::Operator_Equal)
    {
      filterDataWithComparision<std::equal_to<>, T>(input, trueValue, falseValue);
    }
    else if(m_ComparisonOperator == ArrayThreshold::ComparisonType::Operator_NotEqual)
    {
      filterDataWithComparision<std::not_equal_to<>, T>(input, trueValue, falseValue);
    }
    else
    {
      std::string errorMessage = fmt::format("MultiThresholdObjects Comparison Operator not understood: '{}'", static_cast<int>(m_ComparisonOperator));
      throw std::runtime_error(errorMessage);
    }
  }

private:
  ArrayThreshold::ComparisonType m_ComparisonOperator;
  ArrayThreshold::ComparisonValue m_ComparisonValue;
  usize m_ComponentIndex = 0;
  std::vector<U>& m_Output;
};

struct ExecuteThresholdHelper
{
  template <typename Type, typename MaskType>
  void operator()(ThresholdFilterHelper<MaskType>& helper, const IDataArray& iDataArray, Type trueValue, Type falseValue)
  {
    const auto& dataStore = iDataArray.template getIDataStoreRefAs<AbstractDataStore<Type>>();
    helper.template filterData<Type>(dataStore, trueValue, falseValue);
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
void InsertThreshold(usize numItems, AbstractDataStore<T>& currentStore, nx::core::IArrayThreshold::UnionOperator unionOperator, std::vector<T>& newArrayPtr, bool inverse, T trueValue, T falseValue)
{
  for(usize i = 0; i < numItems; i++)
  {
    // invert the current comparison if necessary
    if(inverse)
    {
      newArrayPtr[i] = (newArrayPtr[i] == trueValue) ? falseValue : trueValue;
    }

    if(nx::core::IArrayThreshold::UnionOperator::Or == unionOperator)
    {
      currentStore[i] = (currentStore[i] == trueValue || newArrayPtr[i] == trueValue) ? trueValue : falseValue;
    }
    else if(currentStore[i] == falseValue || newArrayPtr[i] == falseValue)
    {
      currentStore[i] = falseValue;
    }
  }
}

template <typename T>
void ThresholdValue(const ArrayThreshold& comparisonValue, const DataStructure& dataStructure, AbstractDataStore<T>& outputResultStore, int32_t& err, bool replaceInput, bool inverse, T trueValue,
                    T falseValue)
{
  // Get the total number of tuples, create and initialize an array with FALSE to use for these results
  size_t totalTuples = outputResultStore.getNumberOfTuples();
  std::vector<T> tempResultVector(totalTuples, falseValue);

  nx::core::ArrayThreshold::ComparisonType compOperator = comparisonValue.getComparisonType();
  nx::core::ArrayThreshold::ComparisonValue compValue = comparisonValue.getComparisonValue();
  nx::core::IArrayThreshold::UnionOperator unionOperator = comparisonValue.getUnionOperator();

  DataPath inputDataArrayPath = comparisonValue.getArrayPath();

  usize componentIndex = comparisonValue.getComponentIndex();

  ThresholdFilterHelper<T> helper(compOperator, compValue, componentIndex, tempResultVector);

  const auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(inputDataArrayPath);

  ExecuteDataFunction(ExecuteThresholdHelper{}, iDataArray.getDataType(), helper, iDataArray, trueValue, falseValue);

  if(replaceInput)
  {
    if(inverse)
    {
      std::reverse(tempResultVector.begin(), tempResultVector.end());
    }
    // copy the temp uint8 vector to the final uint8 result array
    for(size_t i = 0; i < totalTuples; i++)
    {
      outputResultStore[i] = tempResultVector[i];
    }
  }
  else
  {
    // insert into current threshold
    InsertThreshold<T>(totalTuples, outputResultStore, unionOperator, tempResultVector, inverse, trueValue, falseValue);
  }
}

struct ThresholdValueFunctor
{
  template <typename T>
  void operator()(const ArrayThreshold& comparisonValue, const DataStructure& dataStructure, IDataArray& outputResultArray, int32_t& err, bool replaceInput, bool inverse, T trueValue, T falseValue)
  {
    // Traditionally we would do a check to ensure we get a valid pointer, I'm forgoing that check because it
    // was essentially done in the preflight part.
    ThresholdValue(comparisonValue, dataStructure, outputResultArray.template getIDataStoreRefAs<AbstractDataStore<T>>(), err, replaceInput, inverse, trueValue, falseValue);
  }
};

template <typename T>
void ThresholdSet(const ArrayThresholdSet& inputComparisonSet, const DataStructure& dataStructure, AbstractDataStore<T>& outputResultStore, int32_t& err, bool replaceInput, bool inverse, T trueValue,
                  T falseValue)
{
  // Get the total number of tuples, create and initialize an array with FALSE to use for these results
  size_t totalTuples = outputResultStore.getNumberOfTuples();
  std::vector<T> tempResultVector(totalTuples, falseValue);

  bool firstValueFound = false;

  ArrayThresholdSet::CollectionType thresholds = inputComparisonSet.getArrayThresholds();
  for(const std::shared_ptr<IArrayThreshold>& threshold : thresholds)
  {
    const IArrayThreshold* thresholdPtr = threshold.get();
    if(const auto* comparisonSet = dynamic_cast<const ArrayThresholdSet*>(thresholdPtr); comparisonSet != nullptr)
    {
      ThresholdSet<T>(*comparisonSet, dataStructure, outputResultStore, err, !firstValueFound, false, trueValue, falseValue);
      firstValueFound = true;
    }
    else if(const auto* comparisonValue = dynamic_cast<const ArrayThreshold*>(thresholdPtr); comparisonValue != nullptr)
    {
      ThresholdValue<T>(*comparisonValue, dataStructure, outputResultStore, err, !firstValueFound, false, trueValue, falseValue);
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
      outputResultStore[i] = tempResultVector[i];
    }
  }
  else
  {
    // insert into current threshold
    InsertThreshold<T>(totalTuples, outputResultStore, inputComparisonSet.getUnionOperator(), tempResultVector, inverse, trueValue, falseValue);
  }
}

struct ThresholdSetFunctor
{
  template <typename T>
  void operator()(const ArrayThresholdSet& inputComparisonSet, const DataStructure& dataStructure, IDataArray& outputResultArray, int32_t& err, bool replaceInput, bool inverse, T trueValue,
                  T falseValue)
  {
    // Traditionally we would do a check to ensure we get a valid pointer, I'm forgoing that check because it
    // was essentially done in the preflight part.
    ThresholdSet<T>(inputComparisonSet, dataStructure, outputResultArray.template getIDataStoreRefAs<AbstractDataStore<T>>(), err, replaceInput, inverse, trueValue, falseValue);
  }
};
} // namespace

// -----------------------------------------------------------------------------
MultiThresholdObjects::MultiThresholdObjects(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             MultiThresholdObjectsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MultiThresholdObjects::~MultiThresholdObjects() noexcept = default;

// -----------------------------------------------------------------------------
Result<> MultiThresholdObjects::operator()()
{
  auto thresholdsObject = m_InputValues->ArrayThresholdsObject;
  auto maskArrayName = m_InputValues->OutputDataArrayName;
  auto maskArrayType = m_InputValues->CreatedMaskType;
  auto useCustomTrueValue = m_InputValues->UseCustomTrueValue;
  auto useCustomFalseValue = m_InputValues->UseCustomFalseValue;
  auto customTrueValue = m_InputValues->CustomTrueValue;
  auto customFalseValue = m_InputValues->CustomFalseValue;

  float64 trueValue = useCustomTrueValue ? customTrueValue : 1.0;
  float64 falseValue = useCustomFalseValue ? customFalseValue : 0.0;

  bool firstValueFound = false;
  DataPath maskArrayPath = (*thresholdsObject.getRequiredPaths().begin()).replaceName(maskArrayName);
  int32_t err = 0;
  ArrayThresholdSet::CollectionType thresholdSet = thresholdsObject.getArrayThresholds();
  for(const std::shared_ptr<IArrayThreshold>& threshold : thresholdSet)
  {
    const IArrayThreshold* thresholdPtr = threshold.get();
    if(const auto* comparisonSet = dynamic_cast<const ArrayThresholdSet*>(thresholdPtr); comparisonSet != nullptr)
    {
      ExecuteDataFunction(ThresholdSetFunctor{}, maskArrayType, *comparisonSet, m_DataStructure, m_DataStructure.getDataRefAs<IDataArray>(maskArrayPath), err, !firstValueFound,
                          thresholdsObject.isInverted(), trueValue, falseValue);
      firstValueFound = true;
    }
    else if(const auto* comparisonValue = dynamic_cast<const ArrayThreshold*>(thresholdPtr); comparisonValue != nullptr)
    {
      ExecuteDataFunction(ThresholdValueFunctor{}, maskArrayType, *comparisonValue, m_DataStructure, m_DataStructure.getDataRefAs<IDataArray>(maskArrayPath), err, !firstValueFound,
                          thresholdsObject.isInverted(), trueValue, falseValue);
      firstValueFound = true;
    }
  }

  return {};
}
