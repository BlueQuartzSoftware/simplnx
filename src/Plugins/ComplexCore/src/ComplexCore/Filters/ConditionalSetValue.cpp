#include "ConditionalSetValue.hpp"

#include "complex/Common/ComplexConstants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/TemplateHelpers.hpp"

namespace complex
{

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
  params.insert(std::make_unique<StringParameter>(k_ReplaceValue_Key.str(), "New Value", "", "2.3456789"));
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
  auto pReplaceValue = filterArgs.value<std::string>(k_ReplaceValue_Key.view());
  auto pConditionalArrayPathValue = filterArgs.value<DataPath>(k_ConditionalArrayPath_Key.view());
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key.view());

  if(pReplaceValue.empty())
  {
    return {complex::MakeErrorResult<OutputActions>(-123, fmt::format("{}: Replacement parameter cannot be empty.{}({})", humanName(), __FILE__, __LINE__)), {}};
  }

  const DataObject* inputDataObject = dataStructure.getData(pSelectedArrayPathValue);
  std::string dType = inputDataObject->getTypeName();

  // Sanity check all the inputs here
  Result<> result = complex::CheckValueConvertsToArrayType(pReplaceValue, inputDataObject);
  // We can do this because nothing happens to the DataStructure. *IF* the filter is
  // modifying the DataStructure then we should be using a custom OutputActions instance
  // or hopefully an existing Actions subclass

  // convert the result from above to a Result<OutputActions> object and return. Note the
  // std::move() used for the `result` variable. We can do this because we will *NOT* be
  // using the variable past this line.
  return {complex::CovertResultTo<OutputActions>(std::move(result), {})};
}

Result<> ConditionalSetValue::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pReplaceValue = filterArgs.value<std::string>(k_ReplaceValue_Key.view());
  auto pConditionalArrayPathValue = filterArgs.value<DataPath>(k_ConditionalArrayPath_Key.view());
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key.view());

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  DataObject* inputDataObject = dataStructure.getData(pSelectedArrayPathValue);

  // Assume this works because we made it through preflight, otherwise we will get a nullptr exception very soon afterwards
  DataObject* boolDataObject = dataStructure.getData(pConditionalArrayPathValue);
  BoolArray* conditionalArray = dynamic_cast<BoolArray*>(boolDataObject);

  Result<> result = ConditionalReplaceValueInArray(pReplaceValue, inputDataObject, conditionalArray);

  // EXECUTE_FUNCTION_TEMPLATE(ReplaceValue, result, inputDataObject, data, conditionalArray, pReplaceValue)

  return result;
}
} // namespace complex
