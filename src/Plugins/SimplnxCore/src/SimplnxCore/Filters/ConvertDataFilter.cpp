#include "ConvertDataFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ConvertData.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
const std::string k_TempName = "!!!__INTERNAL_USE_ONLY_temp_INTERNAL_USE_ONLY__!!!";
}

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

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(k_ScalarType_Key, "Scalar Type", "Convert to this data type", 0, GetAllDataTypesAsStrings()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ArrayToConvertPath_Key, "Data Array to Convert", "The complete path to the Data Array to Convert", DataPath{}, GetAllDataTypes()));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_DeleteOriginal_Key, "Perform in Place",
      "If true the original array will be overwritten with the newly typed array, if false the original array will be preserved and the user will be prompted for a name for the new array", false));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_ConvertedArrayName_Key, "Converted Data Array", "The name of the converted Data Array", "Converted_"));

  params.linkParameters(k_DeleteOriginal_Key, k_ConvertedArrayName_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ConvertDataFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertDataFilter::clone() const
{
  return std::make_unique<ConvertDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                          const ExecutionContext& executionContext) const
{
  auto pScalarTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(k_ScalarType_Key);
  auto pInputArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_ArrayToConvertPath_Key);
  auto pConvertedArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ConvertedArrayName_Key);
  auto pRemoveOriginal = filterArgs.value<bool>(k_DeleteOriginal_Key);

  DataPath convertedArrayPath = pInputArrayPath.replaceName(pConvertedArrayName);
  if(pRemoveOriginal)
  {
    convertedArrayPath = pInputArrayPath.replaceName(k_TempName);
  }

  DataType const pScalarType = StringToDataType(GetAllDataTypesAsStrings()[pScalarTypeIndex]);

  Result<OutputActions> resultOutputActions;

  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(pInputArrayPath);

  resultOutputActions.value().appendAction(
      std::make_unique<CreateArrayAction>(pScalarType, inputArray.getIDataStoreRef().getTupleShape(), inputArray.getIDataStoreRef().getComponentShape(), convertedArrayPath));

  if(pRemoveOriginal)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(pInputArrayPath, DeleteDataAction::DeleteType::JustObject));
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(convertedArrayPath, pInputArrayPath.getTargetName()));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ConvertDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ConvertDataInputValues inputValues;
  auto scalarTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(k_ScalarType_Key);
  inputValues.ScalarType = StringToDataType(GetAllDataTypesAsStrings()[scalarTypeIndex]);
  inputValues.SelectedArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_ArrayToConvertPath_Key);
  auto pConvertedArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ConvertedArrayName_Key);
  if(filterArgs.value<bool>(k_DeleteOriginal_Key))
  {
    inputValues.OutputArrayName = inputValues.SelectedArrayPath.replaceName(k_TempName);
  }
  else
  {
    inputValues.OutputArrayName = inputValues.SelectedArrayPath.replaceName(pConvertedArrayName);
  }

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
