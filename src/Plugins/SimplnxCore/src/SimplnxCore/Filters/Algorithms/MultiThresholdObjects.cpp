#include "MultiThresholdObjects.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ArrayThreshold.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>

using namespace nx::core;

namespace
{
/**
 * @brief InsertThreshold is used by ThresholdSets to apply their values to the parent collection using the appropriate union operator and
 * inversion of true/false values.
 * @param currentVector
 * @param unionOperator
 * @param newVector
 * @param inverse
 */
void InsertThreshold(AbstractDataStore<bool>& currentVector, nx::core::IArrayThreshold::UnionOperator unionOperator, AbstractDataStore<bool>& newVector, bool inverse)
{
  usize numItems = currentVector.getNumberOfTuples();

  for(usize i = 0; i < numItems; i++)
  {
    // invert the current comparison if necessary
    if(inverse)
    {
      newVector[i] = !newVector[i];
    }

    if(nx::core::IArrayThreshold::UnionOperator::Or == unionOperator)
    {
      currentVector[i] = (currentVector[i] || newVector[i]);
    }
    else if(!currentVector[i] || !newVector[i])
    {
      currentVector[i] = false;
    }
  }
}

/**
 * @brief Consolidate all assignment calls to a single method to prevent unintended diverging behavior.
 * @param arrayThreshold Current threshold to pull settings from.
 * @param outputResultStore Output DataStore for the current ThresholdSet.
 * @param inputThresholdStore Resulting output for the target array threshold.
 * @param replaceInput The first threshold in every set has its output applied to the output regardless of union operator.
 */
void ApplyThresholdValues(const IArrayThreshold& arrayThreshold, AbstractDataStore<bool>& outputResultStore, AbstractDataStore<bool>& inputThresholdStore, bool replaceInput)
{
  auto unionOperator = arrayThreshold.getUnionOperator();
  bool inverse = arrayThreshold.isInverted();

  if(replaceInput)
  {
    unionOperator = IArrayThreshold::UnionOperator::Or;
  }

  // insert into current threshold
  InsertThreshold(outputResultStore, unionOperator, inputThresholdStore, inverse);
}

class ThresholdFilterHelper
{
public:
  ThresholdFilterHelper(ArrayThreshold::ComparisonType compType, ArrayThreshold::ComparisonValue compValue, usize componentIndex, IArrayThreshold::UnionOperator unionType,
                        AbstractDataStore<bool>& output, bool invert)
  : m_ComparisonOperator(compType)
  , m_ComparisonValue(compValue)
  , m_ComponentIndex(componentIndex)
  , m_UnionType(unionType)
  , m_Output(output)
  , m_Invert(invert)
  {
  }

  template <class CompT, class T>
  void filterDataWithComparision(const AbstractDataStore<T>& inputStore)
  {
    size_t numTuples = inputStore.getNumberOfTuples();
    T value = static_cast<T>(m_ComparisonValue);
    for(size_t tupleIndex = 0; tupleIndex < numTuples; ++tupleIndex)
    {
      T inputValue = inputStore.getComponentValue(tupleIndex, m_ComponentIndex);
      bool currentOutputValue = m_Output.getValue(tupleIndex); // This should only be a single component
      bool comparison = CompT{}(inputValue, value);
      if(m_Invert)
      {
        comparison = !comparison;
      }

      switch(m_UnionType)
      {
      case IArrayThreshold::UnionOperator::And:
        m_Output.setValue(tupleIndex, currentOutputValue && comparison);
        break;
      case IArrayThreshold::UnionOperator::Or:
        m_Output.setValue(tupleIndex, currentOutputValue || comparison);
        break;
      default:
        throw std::runtime_error(fmt::format("Invalid threshold union operator: {}", static_cast<uint8>(m_UnionType)));
        break;
      }
    }
  }

  template <class T>
  void filterData(const AbstractDataStore<T>& input)
  {
    if(m_ComparisonOperator == ArrayThreshold::ComparisonType::LessThan)
    {
      filterDataWithComparision<std::less<>, T>(input);
    }
    else if(m_ComparisonOperator == ArrayThreshold::ComparisonType::GreaterThan)
    {
      filterDataWithComparision<std::greater<>, T>(input);
    }
    else if(m_ComparisonOperator == ArrayThreshold::ComparisonType::Operator_Equal)
    {
      filterDataWithComparision<std::equal_to<>, T>(input);
    }
    else if(m_ComparisonOperator == ArrayThreshold::ComparisonType::Operator_NotEqual)
    {
      filterDataWithComparision<std::not_equal_to<>, T>(input);
    }
    else
    {
      std::string errorMessage = fmt::format("MultiThresholdObjects Comparison Operator not understood: '{}'", static_cast<int32>(m_ComparisonOperator));
      throw std::runtime_error(errorMessage);
    }
  }

private:
  ArrayThreshold::ComparisonType m_ComparisonOperator;
  ArrayThreshold::ComparisonValue m_ComparisonValue;
  usize m_ComponentIndex = 0;
  IArrayThreshold::UnionOperator m_UnionType;
  AbstractDataStore<bool>& m_Output;
  bool m_Invert;
};

struct ExecuteThresholdHelper
{
  template <typename Type>
  void operator()(ThresholdFilterHelper& helper, const IDataArray& iDataArray)
  {
    const auto& dataStore = iDataArray.template getIDataStoreRefAs<AbstractDataStore<Type>>();
    helper.template filterData<Type>(dataStore);
  }
};

void ThresholdValue(const ArrayThreshold& comparisonValue, const DataStructure& dataStructure, AbstractDataStore<bool>& outputResultVector, bool replaceInput)
{
  nx::core::ArrayThreshold::ComparisonType compOperator = comparisonValue.getComparisonType();
  nx::core::ArrayThreshold::ComparisonValue compValue = comparisonValue.getComparisonValue();
  nx::core::IArrayThreshold::UnionOperator unionOperator = comparisonValue.getUnionOperator();

  // Use the Or union operator for the first ThresholdValue in a set.
  if(replaceInput)
  {
    unionOperator = IArrayThreshold::UnionOperator::Or;
  }

  DataPath inputDataArrayPath = comparisonValue.getArrayPath();

  usize componentIndex = comparisonValue.getComponentIndex();

  ThresholdFilterHelper helper(compOperator, compValue, componentIndex, unionOperator, outputResultVector, comparisonValue.isInverted());

  const auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(inputDataArrayPath);

  ExecuteDataFunction(ExecuteThresholdHelper{}, iDataArray.getDataType(), helper, iDataArray);
}

void ThresholdSet(const ArrayThresholdSet& inputComparisonSet, const DataStructure& dataStructure, AbstractDataStore<bool>& outputResultVector, bool replaceInput,
                  const std::atomic_bool& shouldCancel)
{
  // Get the total number of tuples, create and initialize an array with FALSE to use for these results
  size_t totalTuples = outputResultVector.getNumberOfTuples();
  auto tempResultStorePtr = DataStoreUtilities::CreateDataStore<bool>({totalTuples}, {1}, IDataAction::Mode::Execute);
  AbstractDataStore<bool>& tempResultStore = *tempResultStorePtr.get();
  tempResultStore.fill(false);

  bool firstValueFound = false;

  ArrayThresholdSet::CollectionType thresholds = inputComparisonSet.getArrayThresholds();
  for(const std::shared_ptr<IArrayThreshold>& threshold : thresholds)
  {
    if(shouldCancel)
    {
      return;
    }

    const IArrayThreshold* thresholdPtr = threshold.get();
    if(const auto* comparisonSet = dynamic_cast<const ArrayThresholdSet*>(thresholdPtr); comparisonSet != nullptr)
    {
      ThresholdSet(*comparisonSet, dataStructure, tempResultStore, !firstValueFound, shouldCancel);
      firstValueFound = true;
    }
    else if(const auto* comparisonValue = dynamic_cast<const ArrayThreshold*>(thresholdPtr); comparisonValue != nullptr)
    {
      ThresholdValue(*comparisonValue, dataStructure, tempResultStore, !firstValueFound);
      firstValueFound = true;
    }
  }

  // Apply resulting values to output
  ApplyThresholdValues(inputComparisonSet, outputResultVector, tempResultStore, replaceInput);
}

struct ThresholdSetFunctor
{
  template <typename T>
  void operator()(const ArrayThresholdSet& inputComparisonSet, const DataStructure& dataStructure, IDataArray& outputResultArray, bool replaceInput, T trueValue, T falseValue,
                  const std::atomic_bool& shouldCancel)
  {
    if(shouldCancel)
    {
      return;
    }

    // Traditionally we would do a check to ensure we get a valid pointer, I'm forgoing that check because it
    // was essentially done in the preflight part.
    auto& outputDataStore = outputResultArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    usize totalTuples = outputDataStore.getNumberOfTuples();
    auto tempResultStorePtr = DataStoreUtilities::CreateDataStore<bool>({totalTuples}, {1}, IDataAction::Mode::Execute);
    AbstractDataStore<bool>& tempResultStore = *tempResultStorePtr.get();
    ThresholdSet(inputComparisonSet, dataStructure, tempResultStore, err, replaceInput, shouldCancel);

    for(size_t i = 0; i < totalTuples; i++)
    {
      outputDataStore[i] = tempResultStore[i] ? trueValue : falseValue;
    }
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

  if(m_ShouldCancel)
  {
    return {};
  }

  ExecuteDataFunction(ThresholdSetFunctor{}, maskArrayType, thresholdsObject, m_DataStructure, m_DataStructure.getDataRefAs<IDataArray>(maskArrayPath), !firstValueFound, trueValue, falseValue,
                      m_ShouldCancel);

  return {};
}
