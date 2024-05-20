#include "ComputeNumFeaturesFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeNumFeaturesFilter::name() const
{
  return FilterTraits<ComputeNumFeaturesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeNumFeaturesFilter::className() const
{
  return FilterTraits<ComputeNumFeaturesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeNumFeaturesFilter::uuid() const
{
  return FilterTraits<ComputeNumFeaturesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeNumFeaturesFilter::humanName() const
{
  return "Compute Number of Features";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeNumFeaturesFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological", "Find", "Generate", "Calculate", "Determine"};
}

//------------------------------------------------------------------------------
Parameters ComputeNumFeaturesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "Array specifying which Ensemble each Feature belongs",
                                                          DataPath({"DataContainer", "FeatureData", "Phases"}), nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_EnsembleAttributeMatrixPath_Key, "Ensemble Attribute Matrix",
                                                              "The path to the ensemble attribute matrix where the number of features array will be stored",
                                                              DataPath({"DataContainer", "Ensemble Data"}), DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));

  params.insertSeparator(Parameters::Separator{"Output Ensemble Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_NumFeaturesArrayName_Key, "Number of Features", "The number of Features that belong to each Ensemble", "Number of Features"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeNumFeaturesFilter::clone() const
{
  return std::make_unique<ComputeNumFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeNumFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pEnsembleDataPathValue = filterArgs.value<DataPath>(k_EnsembleAttributeMatrixPath_Key);
  auto pNumFeaturesArrayPathValue = pEnsembleDataPathValue.createChildPath(filterArgs.value<std::string>(k_NumFeaturesArrayName_Key));

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto* cellEnsembleData = dataStructure.getDataAs<AttributeMatrix>(pEnsembleDataPathValue);
  if(cellEnsembleData == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-47630, fmt::format("Could not find selected ensemble Attribute Matrix at path '{}'", pEnsembleDataPathValue.toString()))};
  }

  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, cellEnsembleData->getShape(), std::vector<usize>{1}, pNumFeaturesArrayPathValue);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeNumFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pEnsembleDataPathValue = filterArgs.value<DataPath>(k_EnsembleAttributeMatrixPath_Key);
  auto pNumFeaturesArrayPathValue = pEnsembleDataPathValue.createChildPath(filterArgs.value<std::string>(k_NumFeaturesArrayName_Key));

  const auto& featurePhasesArrayRef = dataStructure.getDataRefAs<Int32Array>(pFeaturePhasesArrayPathValue);
  auto& numFeaturesArrayRef = dataStructure.getDataRefAs<Int32Array>(pNumFeaturesArrayPathValue);

  for(usize index = 1; index < featurePhasesArrayRef.getNumberOfTuples(); index++)
  {
    numFeaturesArrayRef[featurePhasesArrayRef[index]]++;
  }
  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_NumFeaturesArrayPathKey = "NumFeaturesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeNumFeaturesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeNumFeaturesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_NumFeaturesArrayPathKey, k_EnsembleAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_NumFeaturesArrayPathKey, k_NumFeaturesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
