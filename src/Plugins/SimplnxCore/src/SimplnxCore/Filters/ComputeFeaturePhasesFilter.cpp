#include "ComputeFeaturePhasesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeFeaturePhases.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeFeaturePhasesFilter::name() const
{
  return FilterTraits<ComputeFeaturePhasesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFeaturePhasesFilter::className() const
{
  return FilterTraits<ComputeFeaturePhasesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeaturePhasesFilter::uuid() const
{
  return FilterTraits<ComputeFeaturePhasesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeaturePhasesFilter::humanName() const
{
  return "Compute Feature Phases";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeaturePhasesFilter::defaultTags() const
{
  return {className(), "Generic", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeaturePhasesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Element belongs", DataPath({"Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeaturesAttributeMatrixPath_Key, "Feature Attribute Matrix",
                                                                    "The AttributeMatrix that stores the feature data for the input **Feature Ids**.", DataPath({"Cell Feature Data"})));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeaturePhasesArrayName_Key, "Feature Phases", "The name of the found feature phases array", "Phases"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeFeaturePhasesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFeaturePhasesFilter::clone() const
{
  return std::make_unique<ComputeFeaturePhasesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeaturePhasesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellFeatureAMPathValue = filterArgs.value<DataPath>(k_CellFeaturesAttributeMatrixPath_Key);
  auto pFeaturePhasesArrayPathValue = pCellFeatureAMPathValue.createChildPath(filterArgs.value<std::string>(k_FeaturePhasesArrayName_Key));

  Result<OutputActions> resultOutputActions;

  const auto& cellFeatData = dataStructure.getDataRefAs<AttributeMatrix>(pCellFeatureAMPathValue);

  {
    auto createFeaturePhasesAction = std::make_unique<CreateArrayAction>(DataType::int32, cellFeatData.getShape(), std::vector<usize>{1}, pFeaturePhasesArrayPathValue);
    resultOutputActions.value().appendAction(std::move(createFeaturePhasesAction));
  }

  const auto* cellPhasesArray = dataStructure.getDataAs<IDataArray>(pCellPhasesArrayPathValue);
  const auto* featureIdsArray = dataStructure.getDataAs<IDataArray>(pFeatureIdsArrayPathValue);
  if(featureIdsArray->getNumberOfTuples() != cellPhasesArray->getNumberOfTuples())
  {
    return MakePreflightErrorResult(-61860, "Size mismatch between cell feature indices and cell phases arrays.");
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeaturePhasesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeFeaturePhasesInputValues inputValues;
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.FeatureIdsPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellFeaturesAttributeMatrixPath = filterArgs.value<DataPath>(k_CellFeaturesAttributeMatrixPath_Key);
  inputValues.FeaturePhasesArrayName = filterArgs.value<std::string>(k_FeaturePhasesArrayName_Key);

  return ComputeFeaturePhases(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeFeaturePhasesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFeaturePhasesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixCreationFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_CellFeaturesAttributeMatrixPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
