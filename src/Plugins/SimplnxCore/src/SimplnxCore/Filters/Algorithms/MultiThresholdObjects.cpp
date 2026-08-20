#include "MultiThresholdObjects.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ArrayThreshold.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <functional>

using namespace nx::core;

namespace
{
/**
 * @brief InsertThreshold is used by ThresholdSets to apply their values to the parent collection using the appropriate union operator and
 * inversion of true/false values.
 * @param currentVector Accumulator for the set being built.
 * @param unionOperator Union operator to combine with. Ignored when replaceOutput is true.
 * @param newVector Values to apply to the accumulator.
 * @param inverse Flip each incoming value before applying it.
 * @param replaceOutput Overwrite the accumulator instead of combining with it. Set for the first child of a set,
 * which has nothing to combine with yet; this is what lets the accumulator skip being pre-filled.
 */
void InsertThreshold(AbstractDataStore<bool>& currentVector, nx::core::IArrayThreshold::UnionOperator unionOperator, const AbstractDataStore<bool>& newVector, bool inverse, bool replaceOutput)
{
  usize numItems = currentVector.getNumberOfTuples();

  // The union operator, the inversion flag, and the replace flag are the same for every tuple, so branch on
  // them once here rather than once per element. Comparing the incoming value against 'inverse' flips it when
  // inversion is requested without a branch inside the loop.
  if(replaceOutput)
  {
    for(usize i = 0; i < numItems; i++)
    {
      currentVector.setValue(i, newVector.getValue(i) != inverse);
    }
  }
  else if(nx::core::IArrayThreshold::UnionOperator::Or == unionOperator)
  {
    for(usize i = 0; i < numItems; i++)
    {
      currentVector.setValue(i, currentVector.getValue(i) || (newVector.getValue(i) != inverse));
    }
  }
  else
  {
    for(usize i = 0; i < numItems; i++)
    {
      currentVector.setValue(i, currentVector.getValue(i) && (newVector.getValue(i) != inverse));
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
void ApplyThresholdValues(const IArrayThreshold& arrayThreshold, AbstractDataStore<bool>& outputResultStore, const AbstractDataStore<bool>& inputThresholdStore, bool replaceInput)
{
  // insert into current threshold
  InsertThreshold(outputResultStore, arrayThreshold.getUnionOperator(), inputThresholdStore, arrayThreshold.isInverted(), replaceInput);
}

class ThresholdFilterHelper
{
public:
  ThresholdFilterHelper(ArrayThreshold::ComparisonType compType, ArrayThreshold::ComparisonValue compValue, usize componentIndex, IArrayThreshold::UnionOperator unionType,
                        AbstractDataStore<bool>& output, bool invert, bool replaceOutput)
  : m_ComparisonOperator(compType)
  , m_ComparisonValue(compValue)
  , m_ComponentIndex(componentIndex)
  , m_UnionType(unionType)
  , m_Output(output)
  , m_Invert(invert)
  , m_ReplaceOutput(replaceOutput)
  {
  }

  template <class CompT, class T>
  void filterDataWithComparision(const AbstractDataStore<T>& inputStore)
  {
    usize numTuples = inputStore.getNumberOfTuples();
    // The comparison value is truncated to the input array's type and the comparison is performed in that
    // type. This matches legacy DREAM3D (SIMPL ThresholdFilterHelper), where a threshold of 5.5 against an
    // int32 array compares against 5, and keeps 64-bit integer comparisons exact.
    const T comparisonValue = static_cast<T>(m_ComparisonValue);

    // The union operator and the invert flag are the same for every tuple, so they are resolved once here
    // instead of inside the loop. m_Output holds a single component per tuple throughout.
    if(m_ReplaceOutput)
    {
      // First threshold in a set: nothing to combine with, so the result is written straight out. This is
      // also why the accumulator does not need to be pre-filled with FALSE.
      for(usize tupleIndex = 0; tupleIndex < numTuples; ++tupleIndex)
      {
        m_Output.setValue(tupleIndex, CompT{}(inputStore.getComponentValue(tupleIndex, m_ComponentIndex), comparisonValue) != m_Invert);
      }
      return;
    }

    if(m_UnionType == IArrayThreshold::UnionOperator::And)
    {
      for(usize tupleIndex = 0; tupleIndex < numTuples; ++tupleIndex)
      {
        m_Output.setValue(tupleIndex, m_Output.getValue(tupleIndex) && (CompT{}(inputStore.getComponentValue(tupleIndex, m_ComponentIndex), comparisonValue) != m_Invert));
      }
      return;
    }

    if(m_UnionType == IArrayThreshold::UnionOperator::Or)
    {
      for(usize tupleIndex = 0; tupleIndex < numTuples; ++tupleIndex)
      {
        m_Output.setValue(tupleIndex, m_Output.getValue(tupleIndex) || (CompT{}(inputStore.getComponentValue(tupleIndex, m_ComponentIndex), comparisonValue) != m_Invert));
      }
      return;
    }

    throw std::runtime_error(fmt::format("Invalid threshold union operator: {}", static_cast<uint8>(m_UnionType)));
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
  bool m_ReplaceOutput;
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

  DataPath inputDataArrayPath = comparisonValue.getArrayPath();

  usize componentIndex = comparisonValue.getComponentIndex();

  // The first ThresholdValue in a set overwrites the accumulator rather than combining with it.
  ThresholdFilterHelper helper(compOperator, compValue, componentIndex, unionOperator, outputResultVector, comparisonValue.isInverted(), replaceInput);

  const auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(inputDataArrayPath);

  ExecuteDataFunction(ExecuteThresholdHelper{}, iDataArray.getDataType(), helper, iDataArray);
}

/**
 * @brief Combines every child of a ThresholdSet into a single boolean result and returns it.
 *
 * The returned store holds the set's *inner* combination only; the set's own union operator and inversion
 * flag are applied by whoever consumes the result, so that the top-level set and nested sets are handled
 * identically. One store is allocated per nesting level and released as the recursion unwinds.
 *
 * @param inputComparisonSet Threshold set whose children should be combined.
 * @param dataStructure DataStructure holding the input arrays.
 * @param totalTuples Number of tuples in the mask being built.
 * @param shouldCancel Cancel flag checked between thresholds.
 * @return Store holding the combined result for this set.
 */
std::shared_ptr<AbstractDataStore<bool>> ComputeThresholdSet(const ArrayThresholdSet& inputComparisonSet, const DataStructure& dataStructure, usize totalTuples, const std::atomic_bool& shouldCancel)
{
  auto resultStorePtr = DataStoreUtilities::CreateDataStore<bool>({totalTuples}, {1}, IDataAction::Mode::Execute);
  AbstractDataStore<bool>& resultStore = *resultStorePtr.get();

  // The first child of the set writes every tuple of the accumulator rather than combining with it, so the
  // store does not need to be pre-filled. The one case that leaves it untouched is a set with no children,
  // handled after the loop; nothing here relies on the store being zero-initialized.
  bool firstValueFound = false;

  ArrayThresholdSet::CollectionType thresholds = inputComparisonSet.getArrayThresholds();
  for(const std::shared_ptr<IArrayThreshold>& threshold : thresholds)
  {
    if(shouldCancel)
    {
      return resultStorePtr;
    }

    const IArrayThreshold* thresholdPtr = threshold.get();
    if(const auto* comparisonSet = dynamic_cast<const ArrayThresholdSet*>(thresholdPtr); comparisonSet != nullptr)
    {
      auto childResultStorePtr = ComputeThresholdSet(*comparisonSet, dataStructure, totalTuples, shouldCancel);
      if(shouldCancel)
      {
        return resultStorePtr;
      }
      ApplyThresholdValues(*comparisonSet, resultStore, *childResultStorePtr.get(), !firstValueFound);
      firstValueFound = true;
    }
    else if(const auto* comparisonValue = dynamic_cast<const ArrayThreshold*>(thresholdPtr); comparisonValue != nullptr)
    {
      ThresholdValue(*comparisonValue, dataStructure, resultStore, !firstValueFound);
      firstValueFound = true;
    }
  }

  if(!firstValueFound)
  {
    // A set with no children contributes nothing; define its result as all-false.
    resultStore.fill(false);
  }

  return resultStorePtr;
}

struct ThresholdSetFunctor
{
  template <typename T>
  void operator()(const ArrayThresholdSet& inputComparisonSet, const DataStructure& dataStructure, IDataArray& outputResultArray, T trueValue, T falseValue, const std::atomic_bool& shouldCancel)
  {
    if(shouldCancel)
    {
      return;
    }

    // Traditionally we would do a check to ensure we get a valid pointer, I'm forgoing that check because it
    // was essentially done in the preflight part.
    auto& outputDataStore = outputResultArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    usize totalTuples = outputDataStore.getNumberOfTuples();
    auto resultStorePtr = ComputeThresholdSet(inputComparisonSet, dataStructure, totalTuples, shouldCancel);
    if(shouldCancel)
    {
      return;
    }
    const AbstractDataStore<bool>& resultStore = *resultStorePtr.get();

    // The top-level set's own inversion flag is applied here, mirroring what ApplyThresholdValues does for a
    // nested set, while the boolean result is converted to the requested mask type.
    const bool inverse = inputComparisonSet.isInverted();
    for(usize i = 0; i < totalTuples; i++)
    {
      outputDataStore.setValue(i, (resultStore.getValue(i) != inverse) ? trueValue : falseValue);
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

  DataPath maskArrayPath = (*thresholdsObject.getRequiredPaths().begin()).replaceName(maskArrayName);

  if(m_ShouldCancel)
  {
    return {};
  }

  ExecuteDataFunction(ThresholdSetFunctor{}, maskArrayType, thresholdsObject, m_DataStructure, m_DataStructure.getDataRefAs<IDataArray>(maskArrayPath), trueValue, falseValue, m_ShouldCancel);

  return {};
}
