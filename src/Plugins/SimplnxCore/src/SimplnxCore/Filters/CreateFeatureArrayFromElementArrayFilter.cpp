#include "CreateFeatureArrayFromElementArrayFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CreateFeatureArrayFromElementArray.hpp"

#include "simplnx/Common/DataTypeUtilities.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
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

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Data to Copy to Feature Data", "The element-level array whose values will be copied up to the Feature level",
                                                          DataPath{}, nx::core::GetAllDataTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix",
                                                                    "The path to the cell feature attribute matrix where the converted output feature array will be stored",
                                                                    DataPath({"DataContainer", "Cell Feature Data"})));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CreatedArrayName_Key, "Created Feature Attribute Array", "The name of the created Feature Attribute Array", ""));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CreateFeatureArrayFromElementArrayFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateFeatureArrayFromElementArrayFilter::clone() const
{
  return std::make_unique<CreateFeatureArrayFromElementArrayFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateFeatureArrayFromElementArrayFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pCreatedArrayNameValue = filterArgs.value<std::string>(k_CreatedArrayName_Key);

  const auto& selectedCellArray = dataStructure.getDataRefAs<IDataArray>(pSelectedCellArrayPathValue);
  const IDataStore& selectedCellArrayStoreRef = selectedCellArray.getIDataStoreRef();

  // The algorithm reads one Feature Id per tuple of the selected element array, so the two
  // arrays must have identical tuple counts or the lookup would run past the end of one of them.
  const auto& featureIdsArray = dataStructure.getDataRefAs<IDataArray>(pFeatureIdsArrayPathValue);
  if(featureIdsArray.getNumberOfTuples() != selectedCellArray.getNumberOfTuples())
  {
    return {MakeErrorResult<OutputActions>(
        -5571, fmt::format("Feature Ids array '{}' has {} tuples but the selected element array '{}' has {} tuples. Both arrays must have the same number of tuples.",
                           pFeatureIdsArrayPathValue.toString(), featureIdsArray.getNumberOfTuples(), pSelectedCellArrayPathValue.toString(), selectedCellArray.getNumberOfTuples()))};
  }

  // The algorithm resizes every array in the destination Attribute Matrix to max(FeatureIds)+1.
  // If that Attribute Matrix is the one holding the input arrays, the resize would truncate or
  // grow the inputs while they are being read — silent data corruption. Reject the selection.
  if(pFeatureIdsArrayPathValue.getParent() == pCellFeatureAttributeMatrixPathValue || pSelectedCellArrayPathValue.getParent() == pCellFeatureAttributeMatrixPathValue)
  {
    return {MakeErrorResult<OutputActions>(-5572, fmt::format("The Feature Attribute Matrix '{}' contains the input arrays. The destination Attribute Matrix is resized to max(Feature Ids) + 1 "
                                                              "tuples during execution, which would corrupt the input arrays. Select a different Feature Attribute Matrix.",
                                                              pCellFeatureAttributeMatrixPathValue.toString()))};
  }

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Create the output array with the destination Attribute Matrix's current tuple shape. The
  // algorithm resizes both to max(FeatureIds) + 1 at execute time, once the values are readable.
  const auto& featureAttributeMatrix = dataStructure.getDataRefAs<AttributeMatrix>(pCellFeatureAttributeMatrixPathValue);
  std::vector<usize> amTupleShape = featureAttributeMatrix.getShape();

  {
    DataType dataType = selectedCellArray.getDataType();
    auto createArrayAction = std::make_unique<CreateArrayAction>(dataType, amTupleShape, selectedCellArrayStoreRef.getComponentShape(),
                                                                 pCellFeatureAttributeMatrixPathValue.createChildPath(pCreatedArrayNameValue), CreateArrayAction::k_DefaultDataFormat, "0");
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CreateFeatureArrayFromElementArrayFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  CreateFeatureArrayFromElementArrayInputValues inputValues;
  inputValues.SelectedCellArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_SelectedCellArrayPath_Key);
  inputValues.FeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellFeatureAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_CellFeatureAttributeMatrixPath_Key);
  inputValues.CreatedArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CreatedArrayName_Key);

  return CreateFeatureArrayFromElementArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CellFeatureAttributeMatrixNameKey = "CellFeatureAttributeMatrixName";
constexpr StringLiteral k_CreatedArrayNameKey = "CreatedArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> CreateFeatureArrayFromElementArrayFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CreateFeatureArrayFromElementArrayFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_SelectedCellArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_CellFeatureAttributeMatrixNameKey,
                                                                                                                         k_CellFeatureAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CreatedArrayNameKey, k_CreatedArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
