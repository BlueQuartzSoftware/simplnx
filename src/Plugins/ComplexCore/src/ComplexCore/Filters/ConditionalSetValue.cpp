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
  params.insert(std::make_unique<StringParameter>(k_ReplaceValue_Key, "New Value", "", "2.3456789"));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ConditionalArrayPath_Key, "Conditional Array", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array", "", DataPath{}));
  return params;
}

IFilter::UniquePointer ConditionalSetValue::clone() const
{
  return std::make_unique<ConditionalSetValue>();
}

IFilter::PreflightResult ConditionalSetValue::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto replaceValueString = filterArgs.value<std::string>(k_ReplaceValue_Key);
  auto selectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);

  if(replaceValueString.empty())
  {
    return {MakeErrorResult<OutputActions>(-123, fmt::format("{}: Replacement parameter cannot be empty.{}({})", humanName(), __FILE__, __LINE__)), {}};
  }

  const DataObject& inputDataObject = dataStructure.getDataRef(selectedArrayPath);

  // Sanity check all the inputs here
  Result<> result = CheckValueConvertsToArrayType(replaceValueString, inputDataObject);
  // We can do this because nothing happens to the DataStructure. *IF* the filter is
  // modifying the DataStructure then we should be using a custom OutputActions instance
  // or hopefully an existing Actions subclass

  // convert the result from above to a Result<OutputActions> object and return. Note the
  // std::move() used for the `result` variable. We can do this because we will *NOT* be
  // using the variable past this line.
  return {ConvertResultTo<OutputActions>(std::move(result), {})};
}

Result<> ConditionalSetValue::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto replaceValueString = filterArgs.value<std::string>(k_ReplaceValue_Key);
  auto conditionalArrayPath = filterArgs.value<DataPath>(k_ConditionalArrayPath_Key);
  auto selectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);

  DataObject& inputDataObject = dataStructure.getDataRef(selectedArrayPath);

  const BoolArray& conditionalArray = dataStructure.getDataRefAs<BoolArray>(conditionalArrayPath);

  Result<> result = ConditionalReplaceValueInArray(replaceValueString, inputDataObject, conditionalArray);

  return result;
}
} // namespace complex
