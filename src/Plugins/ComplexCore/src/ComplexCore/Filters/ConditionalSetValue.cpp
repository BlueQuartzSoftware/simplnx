#include "ConditionalSetValue.hpp"

#include "complex/Common/ComplexConstants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Utilities/TemplateHelpers.hpp"

namespace complex
{

// -----------------------------------------------------------------------------
template <typename T>
Result<> checkValuesInt(float64 replaceValue, const std::string& strType)
{
  std::string ss;
  if(!((replaceValue >= std::numeric_limits<T>::min()) && (replaceValue <= std::numeric_limits<T>::max())))
  {
    return complex::MakeErrorResult<>(-100, fmt::format("The {} replace value was invalid. The valid range is {} to {}", strType, std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
  }
  return {};
}

// -----------------------------------------------------------------------------
template <typename T>
Result<> checkValuesFloatDouble(float64 replaceValue, const std::string& strType)
{
  std::string ss;

  if(!(((replaceValue >= static_cast<T>(-1) * std::numeric_limits<T>::max()) && (replaceValue <= static_cast<T>(-1) * std::numeric_limits<T>::min())) || (replaceValue == 0) ||
       ((replaceValue >= std::numeric_limits<T>::min()) && (replaceValue <= std::numeric_limits<T>::max()))))
  {
    return complex::MakeErrorResult<>(-101, fmt::format("The {} replace value was invalid. The valid ranges are -{} to -{}, 0, %{} to %{}", std::numeric_limits<T>::max(), strType,
                                                        std::numeric_limits<T>::min(), std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
  }
  return {};
}

// -----------------------------------------------------------------------------
template <typename T>
void replaceValue(DataArray<T>* inputArrayPtr, const BoolArray* condDataPtr, double replaceValue)
{
  T replaceVal = static_cast<T>(replaceValue);
  size_t numTuples = inputArrayPtr->getNumberOfTuples();

  for(size_t tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
  {
    if((*condDataPtr)[tupleIndex])
    {
      inputArrayPtr->initializeTuple(tupleIndex, replaceValue);
    }
  }
}

std::string ConditionalSetValue::name() const
{
  return FilterTraits<ConditionalSetValue>::name.str();
}

std::string ConditionalSetValue::className() const
{
  return FilterTraits<ConditionalSetValue>::className;
}

Uuid ConditionalSetValue::uuid() const
{
  return FilterTraits<ConditionalSetValue>::uuid;
}

std::string ConditionalSetValue::humanName() const
{
  return "Replace Value in Array (Conditional)";
}

Parameters ConditionalSetValue::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_ReplaceValue_Key.str(), "New Value", "", 2.3456789));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ConditionalArrayPath_Key.str(), "Conditional Array", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key.str(), "Attribute Array", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ConditionalSetValue::clone() const
{
  return std::make_unique<ConditionalSetValue>();
}

IFilter::PreflightResult ConditionalSetValue::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pReplaceValueValue = filterArgs.value<float64>(k_ReplaceValue_Key.view());
  auto pConditionalArrayPathValue = filterArgs.value<DataPath>(k_ConditionalArrayPath_Key.view());
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key.view());

  const DataObject* inputDataObject = dataStructure.getData(pSelectedArrayPathValue);
  std::string dType = inputDataObject->getTypeName();

  Result<> result;

  VALIDATE_NUMERIC_TYPE(inputDataObject, pReplaceValueValue, result)

  // No changes are actually produced in the DataStructure. We are replacing values
  // in a DataArray instead...
  OutputActions actions;

  return {std::move(actions)};
}

Result<> ConditionalSetValue::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pReplaceValueValue = filterArgs.value<float64>(k_ReplaceValue_Key.view());
  auto pConditionalArrayPathValue = filterArgs.value<DataPath>(k_ConditionalArrayPath_Key.view());
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key.view());

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  DataObject* inputDataObject = dataStructure.getData(pSelectedArrayPathValue);

  // Assume this works because we made it through preflight, otherwise we will get a nullptr exception very soon afterwards
  DataObject* boolDataObject = dataStructure.getData(pConditionalArrayPathValue);
  BoolArray* conditionalArray = dynamic_cast<BoolArray*>(boolDataObject);

  Result<> result;

  EXECUTE_FUNCTION_TEMPLATE(complex::replaceValue, result, inputDataObject, data, conditionalArray, pReplaceValueValue)

  return result;
}
} // namespace complex
