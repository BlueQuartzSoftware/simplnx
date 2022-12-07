#include "ConditionalSetValue.hpp"

#include "complex/Common/ComplexConstants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

using namespace complex;

namespace
{
constexpr int32 k_EmptyParameterValue = -123;
constexpr int32 k_IncorrectInputArrayType = -124;
constexpr int32 k_ConvertReplaceValueTypeError = -125;

template <typename ScalarType>
ScalarType convertFromStringToType(const std::string& convertValue)
{
  Result<ScalarType> convertResult = ConvertTo<ScalarType>::convert(convertValue);

  if(convertResult.invalid())
  {
    throw std::runtime_error(fmt::format("{}({}): Function {}: Error. Cannot convert {} to the type {}.", "ReplaceValueInArrayFunctor", __FILE__, __LINE__, convertValue, ScalarType));
  }

  return static_cast<ScalarType>(convertResult.value());
}

struct ReplaceValueInArrayFunctor
{
  template <typename ScalarType>
  void replaceValue(IDataArray& workingArray, const std::string& removeValue, const std::string& replaceValue)
  {
    DataArray<ScalarType>& dataArray = dynamic_cast<DataArray<ScalarType>&>(workingArray);

    auto removeVal = convertFromStringToType<ScalarType>(removeValue);
    auto replaceVal = convertFromStringToType<ScalarType>(replaceValue);

    const size = dataArray.getNumberOfTuples() * dataArray.getNumberOfComponents();

    for(usize index = 0; index < size; index++)
    {
      if(dataArray[index] == removeVal)
      {
        dataArray[index] = replaceVal;
      }
    }
  }
};
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

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseConditional_Key, "Use Conditional", "Determines whether a conditional array should be used.", false));
  params.insert(std::make_unique<StringParameter>(k_RemoveValue_Key, "Value To Replace", "The value that will be replaced", "0"));
  params.insert(std::make_unique<StringParameter>(k_ReplaceValue_Key, "New Value", "The value that will be used as the replacement value", "0"));
  params.insertSeparator(Parameters::Separator{"Required Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_ConditionalArrayPath_Key, "Conditional Array", "The complete path to the conditional array that will determine which values/entries will be replaced", DataPath{},
      ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8, DataType::int8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array", "The complete path to array that will have values replaced", DataPath{}, complex::GetAllDataTypes()));

  params.linkParameters(k_UseConditional_Key, k_RemoveValue_Key, false);
  params.linkParameters(k_UseConditional_Key, k_ConditionalArrayPath_Key, true);
  return params;
}

IFilter::UniquePointer ConditionalSetValue::clone() const
{
  return std::make_unique<ConditionalSetValue>();
}

IFilter::PreflightResult ConditionalSetValue::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto useConditionalValue = filterArgs.value<bool>(k_ConditionalArrayPath_Key);
  auto removeValueString = filterArgs.value<std::string>(k_RemoveValue_Key);
  auto replaceValueString = filterArgs.value<std::string>(k_ReplaceValue_Key);
  auto selectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pConditionalPath = filterArgs.value<DataPath>(k_ConditionalArrayPath_Key);

  const DataObject& inputDataObject = dataStructure.getDataRef(selectedArrayPath);

  if(replaceValueString.empty())
  {
    return {MakeErrorResult<OutputActions>(::k_EmptyParameterValue, fmt::format("{}: Replacement parameter cannot be empty.{}({})", humanName(), __FILE__, __LINE__)), {}};
  }
  if(removeValueString.empty())
  {
    return {MakeErrorResult<OutputActions>(::k_EmptyParameterValue, fmt::format("{}: Remove parameter cannot be empty.{}({})", humanName(), __FILE__, __LINE__)), {}};
  }

  if(useConditionalValue)
  {
    // Validate that the Conditional Array is of the correct type
    const IDataArray* dataObject = dataStructure.getDataAs<IDataArray>(pConditionalPath);

    if(dataObject->getDataType() != complex::DataType::boolean && dataObject->getDataType() != complex::DataType::uint8 && dataObject->getDataType() != complex::DataType::int8)
    {
      return {MakeErrorResult<OutputActions>(
          ::k_IncorrectInputArrayType, fmt::format("Conditional Array must be of type [Bool|UInt8|Int8]. The object at path '{}' is '{}'", pConditionalPath.toString(), dataObject->getTypeName()))};
    }
  }

  // Sanity check all the inputs here
  Result<> result = CheckValueConvertsToArrayType(replaceValueString, inputDataObject);
  // We can do this because nothing happens to the DataStructure. *IF* the filter is
  // modifying the DataStructure then we should be using a custom OutputActions instance
  // or hopefully an existing Actions subclass

  if(result.invalid())
  {
    return {MakeErrorResult<OutputActions>(::k_ConvertReplaceValueTypeError, fmt::format("{}({}): Function {}: Error. Cannot convert {} to the type {}.", "ReplaceValueInArrayFunctor", __FILE__,
                                                                                         __LINE__, replaceValueString, inputDataObject.getDataObjectType()))};
  }

  result = CheckValueConvertsToArrayType(removeValueString, inputDataObject);

  // convert the result from above to a Result<OutputActions> object and return. Note the
  // std::move() used for the `result` variable. We can do this because we will *NOT* be
  // using the variable past this line.
  return {ConvertResultTo<OutputActions>(std::move(result), {})};
}

Result<> ConditionalSetValue::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  auto useConditionalValue = filterArgs.value<bool>(k_ConditionalArrayPath_Key);
  auto removeValueString = filterArgs.value<std::string>(k_RemoveValue_Key);
  auto replaceValueString = filterArgs.value<std::string>(k_ReplaceValue_Key);
  auto conditionalArrayPath = filterArgs.value<DataPath>(k_ConditionalArrayPath_Key);
  auto selectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);

  if(useConditionalValue)
  {
    DataObject& inputDataObject = dataStructure.getDataRef(selectedArrayPath);

    const IDataArray& conditionalArray = dataStructure.getDataRefAs<IDataArray>(conditionalArrayPath);

    Result<> result = ConditionalReplaceValueInArray(replaceValueString, inputDataObject, conditionalArray);

    return result;
  }
  else
  {
    auto& inputDataArray = dataStructure.getDataRefAs<IDataArray>(selectedArrayPath);
    ExecuteDataFunction(ReplaceValueInArrayFunctor{}, inputDataArray.getDataType(), inputDataArray, removeValueString, replaceValueString);
  }
}
} // namespace complex
