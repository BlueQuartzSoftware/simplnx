#include "ConditionalSetValue.hpp"

#include "complex/Common/ComplexConstants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace
{
constexpr int32 k_EmptyParameterValue = -123;
constexpr int32 k_IncorrectInputArrayType = -124;
} // namespace

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
//------------------------------------------------------------------------------
std::vector<std::string> ConditionalSetValue::defaultTags() const
{
  return {"#Core", "#Processing", "#DataArray"};
}

Parameters ConditionalSetValue::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<StringParameter>(k_ReplaceValue_Key, "New Value", "The value that will be used as the replacement value", ""));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ConditionalArrayPath_Key, "Conditional Array",
                                                          "The complete path to the conditional array that will determine which values/entries will be replaced", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8, DataType::int8}, false));
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array", "The complete path to array that will have values replaced", DataPath{}, complex::GetAllDataTypes()));
  return params;
}

IFilter::UniquePointer ConditionalSetValue::clone() const
{
  return std::make_unique<ConditionalSetValue>();
}

IFilter::PreflightResult ConditionalSetValue::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto replaceValueString = filterArgs.value<std::string>(k_ReplaceValue_Key);
  auto selectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pConditionalPath = filterArgs.value<DataPath>(k_ConditionalArrayPath_Key);

  if(replaceValueString.empty())
  {
    return {MakeErrorResult<OutputActions>(::k_EmptyParameterValue, fmt::format("{}: Replacement parameter cannot be empty.{}({})", humanName(), __FILE__, __LINE__)), {}};
  }

  // Validate that the Conditional Array is of the correct type
  const IDataArray* dataObject = dataStructure.getDataAs<IDataArray>(pConditionalPath);

  if(dataObject->getDataType() != complex::DataType::boolean && dataObject->getDataType() != complex::DataType::uint8 && dataObject->getDataType() != complex::DataType::int8)
  {
    return {MakeErrorResult<OutputActions>(
        ::k_IncorrectInputArrayType, fmt::format("Conditional Array must be of type [Bool|UInt8|Int8]. The object at path '{}' is '{}'", pConditionalPath.toString(), dataObject->getTypeName()))};
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

Result<> ConditionalSetValue::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  auto replaceValueString = filterArgs.value<std::string>(k_ReplaceValue_Key);
  auto conditionalArrayPath = filterArgs.value<DataPath>(k_ConditionalArrayPath_Key);
  auto selectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);

  DataObject& inputDataObject = dataStructure.getDataRef(selectedArrayPath);

  const IDataArray& conditionalArray = dataStructure.getDataRefAs<IDataArray>(conditionalArrayPath);

  Result<> result = ConditionalReplaceValueInArray(replaceValueString, inputDataObject, conditionalArray);

  return result;
}
} // namespace complex
