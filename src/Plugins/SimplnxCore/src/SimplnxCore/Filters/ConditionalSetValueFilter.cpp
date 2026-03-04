#include "ConditionalSetValueFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ConditionalSetValue.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"

using namespace nx::core;

namespace
{
constexpr int32 k_EmptyParameterValue = -123;
constexpr int32 k_IncorrectInputArrayType = -124;
constexpr int32 k_ConvertReplaceValueTypeError = -125;
constexpr int32 k_ConvertRemoveValueTypeError = -126;
}; // namespace

namespace nx::core
{

std::string ConditionalSetValueFilter::name() const
{
  return FilterTraits<ConditionalSetValueFilter>::name.str();
}

std::string ConditionalSetValueFilter::className() const
{
  return FilterTraits<ConditionalSetValueFilter>::className;
}

Uuid ConditionalSetValueFilter::uuid() const
{
  return FilterTraits<ConditionalSetValueFilter>::uuid;
}

std::string ConditionalSetValueFilter::humanName() const
{
  return "Replace Value in Array (Conditional)";
}
//------------------------------------------------------------------------------
std::vector<std::string> ConditionalSetValueFilter::defaultTags() const
{
  return {className(), "Core", "Processing", "DataArray"};
}

Parameters ConditionalSetValueFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<StringParameter>(k_ReplaceValue_Key, "New Value", "The value that will be used as the replacement value", "0"));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseConditional_Key, "Use Conditional Mask", "Whether to use a boolean mask array to replace values marked TRUE", false));
  params.insert(std::make_unique<BoolParameter>(k_InvertMask_Key, "Invert Mask", "This makes the filter replace values that are marked FALSE in the conditional array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_ConditionalArrayPath_Key, "Conditional Array", "The complete path to the conditional array that will determine which values/entries will be replaced if index is TRUE", DataPath{},
      ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8, DataType::int8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<StringParameter>(k_RemoveValue_Key, "Value To Replace", "The numerical value that will be replaced in the array", "0"));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array", "The complete path to array that will have values replaced", DataPath{}, nx::core::GetAllDataTypes()));

  params.linkParameters(k_UseConditional_Key, k_RemoveValue_Key, false);
  params.linkParameters(k_UseConditional_Key, k_ConditionalArrayPath_Key, true);
  params.linkParameters(k_UseConditional_Key, k_InvertMask_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ConditionalSetValueFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer ConditionalSetValueFilter::clone() const
{
  return std::make_unique<ConditionalSetValueFilter>();
}

IFilter::PreflightResult ConditionalSetValueFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto useConditionalValue = filterArgs.value<bool>(k_UseConditional_Key);
  auto removeValueString = filterArgs.value<std::string>(k_RemoveValue_Key);
  auto replaceValueString = filterArgs.value<std::string>(k_ReplaceValue_Key);
  auto selectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pConditionalPath = filterArgs.value<DataPath>(k_ConditionalArrayPath_Key);

  const auto& inputDataObject = dataStructure.getDataAs<AbstractDataArray>(selectedArrayPath);

  if(replaceValueString.empty())
  {
    return MakePreflightErrorResult(::k_EmptyParameterValue, fmt::format("{}: Replacement parameter cannot be empty.{}({})", humanName(), __FILE__, __LINE__));
  }
  if(removeValueString.empty())
  {
    return MakePreflightErrorResult(::k_EmptyParameterValue, fmt::format("{}: Remove parameter cannot be empty.{}({})", humanName(), __FILE__, __LINE__));
  }

  if(useConditionalValue)
  {
    // Validate that the Conditional Array is of the correct type
    const auto* dataObject = dataStructure.getDataAs<AbstractDataArray>(pConditionalPath);

    if(dataObject->getDataType() != nx::core::DataType::boolean && dataObject->getDataType() != nx::core::DataType::uint8 && dataObject->getDataType() != nx::core::DataType::int8)
    {
      return MakePreflightErrorResult(::k_IncorrectInputArrayType,
                                      fmt::format("Conditional Array must be of type [Bool|UInt8|Int8]. The object at path '{}' is '{}'", pConditionalPath.toString(), dataObject->getTypeName()));
    }
  }

  // Sanity check all the inputs here
  Result<> result = StringInterpretationUtilities::CheckValueConverts(inputDataObject->getDataType(), replaceValueString);
  if(result.invalid())
  {
    return MakePreflightErrorResult(::k_ConvertReplaceValueTypeError, fmt::format("{}({}): Function {}: Error. Cannot convert replace value {} to the type {}.", "ReplaceValueInArrayFunctor", __FILE__,
                                                                                  __LINE__, replaceValueString, fmt::underlying(inputDataObject->getDataObjectType())));
  }

  result = StringInterpretationUtilities::CheckValueConverts(inputDataObject->getDataType(), removeValueString);
  if(result.invalid())
  {
    return MakePreflightErrorResult(::k_ConvertRemoveValueTypeError, fmt::format("{}({}): Function {}: Error. Cannot convert remove value {} to the type {}.", "ReplaceValueInArrayFunctor", __FILE__,
                                                                                 __LINE__, removeValueString, fmt::underlying(inputDataObject->getDataObjectType())));
  }

  nx::core::Result<OutputActions> resultOutputActions;

  // Inform users that the following arrays are going to be modified in place
  nx::core::MarkDataPathModified(dataStructure, resultOutputActions, selectedArrayPath);

  return {std::move(resultOutputActions)};
}

Result<> ConditionalSetValueFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ConditionalSetValueInputValues inputValues;
  inputValues.ConditionalArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_ConditionalArrayPath_Key);
  inputValues.InvertMask = filterArgs.value<BoolParameter::ValueType>(k_InvertMask_Key);
  inputValues.RemoveValue = filterArgs.value<StringParameter::ValueType>(k_RemoveValue_Key);
  inputValues.ReplaceValue = filterArgs.value<StringParameter::ValueType>(k_ReplaceValue_Key);
  inputValues.SelectedArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_SelectedArrayPath_Key);
  inputValues.UseConditional = filterArgs.value<BoolParameter::ValueType>(k_UseConditional_Key);

  return ConditionalSetValue(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_RemoveValueKey = "RemoveValue";
constexpr StringLiteral k_ReplaceValueKey = "ReplaceValue";
constexpr StringLiteral k_SelectedArrayKey = "SelectedArray";
constexpr StringLiteral k_ConditionalArrayPathKey = "ConditionalArrayPath";
constexpr StringLiteral k_SelectedArrayPathKey = "SelectedArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ConditionalSetValueFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ConditionalSetValueFilter().getDefaultArguments();

  std::vector<Result<>> results;

  if(json.contains(SIMPL::k_RemoveValueKey))
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleToStringFilterParameterConverter>(args, json, SIMPL::k_RemoveValueKey, k_RemoveValue_Key));
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedArrayKey, k_SelectedArrayPath_Key));
  }
  else
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ConditionalArrayPathKey, k_ConditionalArrayPath_Key));
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedArrayPathKey, k_SelectedArrayPath_Key));
  }
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleToStringFilterParameterConverter>(args, json, SIMPL::k_ReplaceValueKey, k_ReplaceValue_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
