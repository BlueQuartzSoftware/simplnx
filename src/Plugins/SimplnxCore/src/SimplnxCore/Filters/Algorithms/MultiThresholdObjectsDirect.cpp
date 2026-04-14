#include "MultiThresholdObjectsDirect.hpp"

#include "MultiThresholdObjects.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ArrayThreshold.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>

using namespace nx::core;

// =============================================================================
// MultiThresholdObjectsDirect — In-Core Algorithm
//
// This file implements the in-core (Direct) variant of MultiThresholdObjects.
// It is selected by DispatchAlgorithm when all input arrays reside in memory.
//
// ALGORITHM OVERVIEW:
//   For each threshold condition in the user-defined threshold tree:
//   1. Allocate an O(n) temporary result vector initialized to FALSE
//   2. Read each element of the input array via getComponentValue()
//   3. Apply the comparison (< > == !=) to produce TRUE/FALSE per element
//   4. Merge the temporary results into the output mask using AND/OR logic
//
//   Threshold conditions can be nested in ArrayThresholdSets (which recursively
//   apply AND/OR between their children) or be individual ArrayThreshold comparisons.
//
// DATA ACCESS PATTERN:
//   Uses getComponentValue() for per-element random access to input arrays, and
//   operator[] for per-element writes to the output mask and temporary vectors.
//   This is optimal for in-memory data. The O(n) temporary vector is acceptable
//   when data is in memory but would be wasteful for OOC data — see the Scanline
//   variant which uses O(chunkSize) temporaries instead.
// =============================================================================

namespace
{
/**
 * @brief Helper class that applies a single threshold comparison to an input array
 * and writes TRUE/FALSE results into an output vector.
 *
 * @tparam U The output mask element type (e.g., uint8, bool, float32)
 */
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
    usize numTuples = m_Input.getNumberOfTuples();
    T value = static_cast<T>(m_ComparisonValue);
    for(usize tupleIndex = 0; tupleIndex < numTuples; ++tupleIndex)
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
void ThresholdValue(const ArrayThreshold& comparisonValue, const DataStructure& dataStructure, AbstractDataStore<T>& outputResultStore, int32& err, bool replaceInput, bool inverse, T trueValue,
                    T falseValue, const std::atomic_bool& shouldCancel)
{
  // Get the total number of tuples, create and initialize an array with FALSE to use for these results
  usize totalTuples = outputResultStore.getNumberOfTuples();
  std::vector<T> tempResultVector(totalTuples, falseValue);

  nx::core::ArrayThreshold::ComparisonType compOperator = comparisonValue.getComparisonType();
  nx::core::ArrayThreshold::ComparisonValue compValue = comparisonValue.getComparisonValue();
  nx::core::IArrayThreshold::UnionOperator unionOperator = comparisonValue.getUnionOperator();

  DataPath inputDataArrayPath = comparisonValue.getArrayPath();

  usize componentIndex = comparisonValue.getComponentIndex();

  ThresholdFilterHelper<T> helper(compOperator, compValue, componentIndex, tempResultVector);

  const auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(inputDataArrayPath);

  ExecuteDataFunction(ExecuteThresholdHelper{}, iDataArray.getDataType(), helper, iDataArray, trueValue, falseValue);

  if(shouldCancel)
  {
    return;
  }

  if(replaceInput)
  {
    if(inverse)
    {
      std::reverse(tempResultVector.begin(), tempResultVector.end());
    }
    // copy the temp uint8 vector to the final uint8 result array
    for(usize i = 0; i < totalTuples; i++)
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
  void operator()(const ArrayThreshold& comparisonValue, const DataStructure& dataStructure, IDataArray& outputResultArray, int32& err, bool replaceInput, bool inverse, T trueValue, T falseValue,
                  const std::atomic_bool& shouldCancel)
  {
    ThresholdValue(comparisonValue, dataStructure, outputResultArray.template getIDataStoreRefAs<AbstractDataStore<T>>(), err, replaceInput, inverse, trueValue, falseValue, shouldCancel);
  }
};

template <typename T>
void ThresholdSet(const ArrayThresholdSet& inputComparisonSet, const DataStructure& dataStructure, AbstractDataStore<T>& outputResultStore, int32& err, bool replaceInput, bool inverse, T trueValue,
                  T falseValue, const std::atomic_bool& shouldCancel)
{
  // Get the total number of tuples, create and initialize an array with FALSE to use for these results
  usize totalTuples = outputResultStore.getNumberOfTuples();
  std::vector<T> tempResultVector(totalTuples, falseValue);

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
      ThresholdSet<T>(*comparisonSet, dataStructure, outputResultStore, err, !firstValueFound, false, trueValue, falseValue, shouldCancel);
      firstValueFound = true;
    }
    else if(const auto* comparisonValue = dynamic_cast<const ArrayThreshold*>(thresholdPtr); comparisonValue != nullptr)
    {
      ThresholdValue<T>(*comparisonValue, dataStructure, outputResultStore, err, !firstValueFound, false, trueValue, falseValue, shouldCancel);
      firstValueFound = true;
    }
  }

  if(shouldCancel)
  {
    return;
  }

  if(replaceInput)
  {
    if(inverse)
    {
      std::reverse(tempResultVector.begin(), tempResultVector.end());
    }
    // copy the temp uint8 vector to the final uint8 result array
    for(usize i = 0; i < totalTuples; i++)
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
  void operator()(const ArrayThresholdSet& inputComparisonSet, const DataStructure& dataStructure, IDataArray& outputResultArray, int32& err, bool replaceInput, bool inverse, T trueValue,
                  T falseValue, const std::atomic_bool& shouldCancel)
  {
    ThresholdSet<T>(inputComparisonSet, dataStructure, outputResultArray.template getIDataStoreRefAs<AbstractDataStore<T>>(), err, replaceInput, inverse, trueValue, falseValue, shouldCancel);
  }
};
} // namespace

// -----------------------------------------------------------------------------
MultiThresholdObjectsDirect::MultiThresholdObjectsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         const MultiThresholdObjectsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MultiThresholdObjectsDirect::~MultiThresholdObjectsDirect() noexcept = default;

// -----------------------------------------------------------------------------
Result<> MultiThresholdObjectsDirect::operator()()
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
  ArrayThresholdSet::CollectionType ThresholdSet = thresholdsObject.getArrayThresholds();

  usize numThresholds = ThresholdSet.size();
  const auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(maskArrayPath);
  usize totalTuples = maskArray.getNumberOfTuples();

  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Applying {} threshold{} to {} tuples...", numThresholds, numThresholds == 1 ? "" : "s", totalTuples));

  bool firstValueFound = false;
  int32 err = 0;
  for(usize threshIdx = 0; threshIdx < ThresholdSet.size(); threshIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto& threshold = ThresholdSet[threshIdx];
    const IArrayThreshold* thresholdPtr = threshold.get();
    if(const auto* comparisonSet = dynamic_cast<const ArrayThresholdSet*>(thresholdPtr); comparisonSet != nullptr)
    {
      ExecuteDataFunction(ThresholdSetFunctor{}, maskArrayType, *comparisonSet, m_DataStructure, m_DataStructure.getDataRefAs<IDataArray>(maskArrayPath), err, !firstValueFound,
                          thresholdsObject.isInverted(), trueValue, falseValue, m_ShouldCancel);
      firstValueFound = true;
    }
    else if(const auto* comparisonValue = dynamic_cast<const ArrayThreshold*>(thresholdPtr); comparisonValue != nullptr)
    {
      ExecuteDataFunction(ThresholdValueFunctor{}, maskArrayType, *comparisonValue, m_DataStructure, m_DataStructure.getDataRefAs<IDataArray>(maskArrayPath), err, !firstValueFound,
                          thresholdsObject.isInverted(), trueValue, falseValue, m_ShouldCancel);
      firstValueFound = true;
    }
  }

  return {};
}
