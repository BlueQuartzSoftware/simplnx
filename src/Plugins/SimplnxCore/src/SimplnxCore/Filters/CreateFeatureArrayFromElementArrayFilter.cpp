#include "CreateFeatureArrayFromElementArrayFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CreateFeatureArrayFromElementArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Utilities/DataObjectUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CreateFeatureArrayFromElementArrayFilter::name() const
{
  return FilterTraits<CreateFeatureArrayFromElementArrayFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateFeatureArrayFromElementArrayFilter::className() const
{
  return FilterTraits<CreateFeatureArrayFromElementArrayFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CreateFeatureArrayFromElementArrayFilter::uuid() const
{
  return FilterTraits<CreateFeatureArrayFromElementArrayFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateFeatureArrayFromElementArrayFilter::humanName() const
{
  return "Create Feature Array from Element Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateFeatureArrayFromElementArrayFilter::defaultTags() const
{
  return {className(), "Core", "Memory Management"};
}

//------------------------------------------------------------------------------
Parameters CreateFeatureArrayFromElementArrayFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Data to Copy to Feature Data", "Element Data to Copy to Feature Data", DataPath{}, GetAllDataTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix",
                                                                    "The path to the cell feature attribute matrix where the converted output feature array will be stored",
                                                                    DataPath({"DataContainer", "Cell Feature Data"})));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CreatedArrayName_Key, "Created Feature Attribute Array", "The path to the copied AttributeArray", ""));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CreateFeatureArrayFromElementArrayFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType CreateFeatureArrayFromElementArrayFilter::clone() const
{
  return std::make_unique<CreateFeatureArrayFromElementArrayFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateFeatureArrayFromElementArrayFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                                 const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  const auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  const auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  const auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  const auto pCreatedArrayNameValue = filterArgs.value<std::string>(k_CreatedArrayName_Key);

  const auto& selectedCellArray = dataStructure.getDataRefAs<IDataArray>(pSelectedCellArrayPathValue);
  const IDataStore& selectedCellArrayStore = selectedCellArray.getIDataStoreRef();

  const auto& featureIdsArray = dataStructure.getDataRefAs<IDataArray>(pFeatureIdsArrayPathValue);

  if(selectedCellArrayStore.getNumberOfTuples() != featureIdsArray.getNumberOfTuples())
  {
    return MakePreflightErrorResult(-81883, fmt::format("Cell array '{}' has {} tuples but FeatureIds array '{}' has {} tuples; they must match.", pSelectedCellArrayPathValue.toString(),
                                                        selectedCellArrayStore.getNumberOfTuples(), pFeatureIdsArrayPathValue.toString(), featureIdsArray.getNumberOfTuples()));
  }

  Result<OutputActions> resultOutputActions;
  const auto& featureAttributeMatrix = dataStructure.getDataRefAs<AttributeMatrix>(pCellFeatureAttributeMatrixPathValue);
  {
    DataType dataType = selectedCellArray.getDataType();
    auto createArrayAction = std::make_unique<CreateArrayAction>(dataType, featureAttributeMatrix.getShape(), selectedCellArrayStore.getComponentShape(),
                                                                 pCellFeatureAttributeMatrixPathValue.createChildPath(pCreatedArrayNameValue), "", "0");
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {.output_actions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> CreateFeatureArrayFromElementArrayFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  CreateFeatureArrayFromElementArrayInputValues inputValues;
  inputValues.selected_cell_array_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_SelectedCellArrayPath_Key);
  inputValues.feature_ids_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_CellFeatureIdsArrayPath_Key);
  inputValues.cell_feature_attribute_matrix_path = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_CellFeatureAttributeMatrixPath_Key);
  inputValues.created_array_name = filterArgs.value<DataObjectNameParameter::ValueType>(k_CreatedArrayName_Key);

  return CreateFeatureArrayFromElementArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_CellFeatureAttributeMatrixNameKey = "CellFeatureAttributeMatrixName";
constexpr StringLiteralType k_CreatedArrayNameKey = "CreatedArrayName";
} // namespace simpl
} // namespace

Result<Arguments> CreateFeatureArrayFromElementArrayFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CreateFeatureArrayFromElementArrayFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_SelectedCellArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, simpl::k_CellFeatureAttributeMatrixNameKey,
                                                                                                                           k_CellFeatureAttributeMatrixPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CreatedArrayNameKey, k_CreatedArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
