#include "CopyFeatureArrayToElementArrayFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CopyFeatureArrayToElementArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
// DataObjectNameParameter.hpp and MultiPathSelectionParameter.hpp provide the SIMPLConversion
// converters (LinkedPathCreation..., SingleToMultiDataPathSelection...) used in FromSIMPLJson()
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/MultiPathSelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{

//------------------------------------------------------------------------------
std::string CopyFeatureArrayToElementArrayFilter::name() const
{
  return FilterTraits<CopyFeatureArrayToElementArrayFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CopyFeatureArrayToElementArrayFilter::className() const
{
  return FilterTraits<CopyFeatureArrayToElementArrayFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CopyFeatureArrayToElementArrayFilter::uuid() const
{
  return FilterTraits<CopyFeatureArrayToElementArrayFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CopyFeatureArrayToElementArrayFilter::humanName() const
{
  return "Create Element Array from Feature Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> CopyFeatureArrayToElementArrayFilter::defaultTags() const
{
  return {className(), "Core", "Memory Management"};
}

//------------------------------------------------------------------------------
Parameters CopyFeatureArrayToElementArrayFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedFeatureArrayPaths_Key, "Feature Data to Copy to Cell Data",
                                                               "The DataPath to the feature data that should be copied to the cell level", MultiArraySelectionParameter::ValueType{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_CreatedArraySuffix_Key, "Created Array Suffix", "The suffix to add to the input attribute array name when creating the copied array", ""));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CopyFeatureArrayToElementArrayFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CopyFeatureArrayToElementArrayFilter::clone() const
{
  return std::make_unique<CopyFeatureArrayToElementArrayFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CopyFeatureArrayToElementArrayFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const auto pSelectedFeatureArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedFeatureArrayPaths_Key);
  const auto pFeatureIdsArrayPathValue = filterArgs.value<ArraySelectionParameter::ValueType>(k_CellFeatureIdsArrayPath_Key);
  const auto pCreatedArraySuffixValue = filterArgs.value<StringParameter::ValueType>(k_CreatedArraySuffix_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(pSelectedFeatureArrayPathsValue.empty())
  {
    return MakePreflightErrorResult(nx::core::FilterParameter::Constants::k_Validate_Empty_Value, "You must select at least one feature data array to copy to an element data array.");
  }

  if(pCreatedArraySuffixValue.find('/') != std::string::npos)
  {
    return MakePreflightErrorResult(-3021, fmt::format("The Created Array Suffix '{}' must not contain a '/' character.", pCreatedArraySuffixValue));
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(pSelectedFeatureArrayPathsValue);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-3020, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  const auto& featureIdsArray = dataStructure.getDataRefAs<IDataArray>(pFeatureIdsArrayPathValue);
  const IDataStore& featureIdsArrayStore = featureIdsArray.getIDataStoreRef();
  const std::vector<usize>& tupleShape = featureIdsArrayStore.getTupleShape();

  for(const auto& selectedFeatureArrayPath : pSelectedFeatureArrayPathsValue)
  {
    DataPath createdArrayPath = pFeatureIdsArrayPathValue.replaceName(selectedFeatureArrayPath.getTargetName() + pCreatedArraySuffixValue);
    const auto& selectedFeatureArray = dataStructure.getDataRefAs<IDataArray>(selectedFeatureArrayPath);
    DataType dataType = selectedFeatureArray.getDataType();
    auto createArrayAction = std::make_unique<CreateArrayAction>(dataType, tupleShape, selectedFeatureArray.getComponentShape(), createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CopyFeatureArrayToElementArrayFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  CopyFeatureArrayToElementArrayInputValues inputValues;
  inputValues.SelectedFeatureArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedFeatureArrayPaths_Key);
  inputValues.FeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CreatedArraySuffix = filterArgs.value<StringParameter::ValueType>(k_CreatedArraySuffix_Key);

  return CopyFeatureArrayToElementArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedFeatureArrayPathKey = "SelectedFeatureArrayPath";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CreatedArrayNameKey = "CreatedArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> CopyFeatureArrayToElementArrayFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CopyFeatureArrayToElementArrayFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::SingleToMultiDataPathSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedFeatureArrayPathKey, k_SelectedFeatureArrayPaths_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  // Do NOT map the legacy CreatedArrayName onto k_CreatedArraySuffix_Key: the legacy filter converted a
  // single array whose output name was CreatedArrayName, but in SIMPLNX that string would be appended to
  // the input array name as a suffix (e.g. "AvgEuler" + "AvgEulerAngles"). Leave the suffix blank so the
  // copied array keeps the input array's name.

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
