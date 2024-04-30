#include "ConvertDataFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ConvertData.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ConvertDataFilter::name() const
{
  return FilterTraits<ConvertDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertDataFilter::className() const
{
  return FilterTraits<ConvertDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertDataFilter::uuid() const
{
  return FilterTraits<ConvertDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertDataFilter::humanName() const
{
  return "Convert AttributeArray DataType";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertDataFilter::defaultTags() const
{
  return {className(), "Core", "Convert"};
}

//------------------------------------------------------------------------------
Parameters ConvertDataFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<ChoicesParameter>(k_ScalarType_Key, "Scalar Type", "Convert to this data type", 0, GetAllDataTypesAsStrings()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ArrayToConvertPath_Key, "Data Array to Convert", "The complete path to the Data Array to Convert", DataPath{}, GetAllDataTypes()));
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginal_Key, "Remove Original Array", "Whether or not to remove the original array after conversion", false));

  params.insertSeparator(Parameters::Separator{"Created Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_ConvertedArrayName_Key, "Converted Data Array", "The name of the converted Data Array", "Converted_"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertDataFilter::clone() const
{
  return std::make_unique<ConvertDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto pScalarTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(k_ScalarType_Key);
  auto pInputArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_ArrayToConvertPath_Key);
  auto pConvertedArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ConvertedArrayName_Key);
  auto pRemoveOriginal = filterArgs.value<bool>(k_DeleteOriginal_Key);

  DataPath const convertedArrayPath = pInputArrayPath.replaceName(pConvertedArrayName);

  DataType const pScalarType = StringToDataType(GetAllDataTypesAsStrings()[pScalarTypeIndex]);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;

  auto* inputArrayPtr = dataStructure.getDataAs<IDataArray>(pInputArrayPath);
  if(inputArrayPtr == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-15201, fmt::format("Cannot find input data array at path '{}'", pInputArrayPath.toString())}})};
  }

  resultOutputActions.value().appendAction(
      std::make_unique<CreateArrayAction>(pScalarType, inputArrayPtr->getIDataStoreRef().getTupleShape(), inputArrayPtr->getIDataStoreRef().getComponentShape(), convertedArrayPath));

  std::vector<PreflightValue> preflightUpdatedValues;

  if(pRemoveOriginal)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(pInputArrayPath, DeleteDataAction::DeleteType::JustObject));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ConvertDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  ConvertDataInputValues inputValues;
  uint64 scalarTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(k_ScalarType_Key);
  inputValues.ScalarType = StringToDataType(GetAllDataTypesAsStrings()[scalarTypeIndex]);
  inputValues.SelectedArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_ArrayToConvertPath_Key);
  auto pConvertedArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ConvertedArrayName_Key);
  inputValues.OutputArrayName = inputValues.SelectedArrayPath.replaceName(pConvertedArrayName);

  return ConvertData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_ScalarTypeKey = "ScalarType";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_OutputArrayNameKey = "OutputArrayName";
} // namespace SIMPL
namespace SIMPLConversionCustom
{
struct LinkedPathCreationFilterParameterConverter
{
  using ParameterType = DataObjectNameParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_string())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("ConvertDataFilter::LinkedPathCreationFilterParameterConverter json '{}' is not a string", json.dump()));
    }

    auto value = json.get<std::string>();

    return {std::move(value)};
  }
};
} // namespace SIMPLConversionCustom
} // namespace

Result<Arguments> ConvertDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ConvertDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_ScalarTypeKey, k_ScalarType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_ArrayToConvertPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversionCustom::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_OutputArrayNameKey, k_ConvertedArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
