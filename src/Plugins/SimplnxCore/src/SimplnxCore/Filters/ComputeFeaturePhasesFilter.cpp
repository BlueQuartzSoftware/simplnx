#include "ComputeFeaturePhasesFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Utilities/DataArrayUtilities.hpp"

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
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Element belongs", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeaturesAttributeMatrixPath_Key, "Feature Attribute Matrix",
                                                                    "The AttributeMatrix that stores the feature data for the input **Feature Ids**.", DataPath({"Cell Feature Data"})));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_FeaturePhasesArrayName_Key, "Feature Phases", "The name of the feature attribute matrix in which to store the found feature phases array", "Phases"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFeaturePhasesFilter::clone() const
{
  return std::make_unique<ComputeFeaturePhasesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeaturePhasesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellFeatureAMPathValue = filterArgs.value<DataPath>(k_CellFeaturesAttributeMatrixPath_Key);
  auto pFeaturePhasesArrayPathValue = pCellFeatureAMPathValue.createChildPath(filterArgs.value<std::string>(k_FeaturePhasesArrayName_Key));

  nx::core::Result<OutputActions> resultOutputActions;

  if(dataStructure.getDataAs<Int32Array>(pFeatureIdsArrayPathValue) == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-4630, fmt::format("Could not find selected feature ids array at path '{}'", pFeatureIdsArrayPathValue.toString()))};
  }
  const auto* cellFeatData = dataStructure.getDataAs<AttributeMatrix>(pCellFeatureAMPathValue);
  if(cellFeatData == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-4631, fmt::format("Could not find selected cell features Attribute Matrix at path '{}'", pCellFeatureAMPathValue.toString()))};
  }

  {
    auto createFeaturePhasesAction = std::make_unique<CreateArrayAction>(DataType::int32, cellFeatData->getShape(), std::vector<usize>{1}, pFeaturePhasesArrayPathValue);
    resultOutputActions.value().appendAction(std::move(createFeaturePhasesAction));
  }

  std::vector<PreflightValue> preflightUpdatedValues;

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeaturePhasesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellFeatureAMPathValue = filterArgs.value<DataPath>(k_CellFeaturesAttributeMatrixPath_Key);
  auto pFeaturePhasesArrayPathValue = pCellFeatureAMPathValue.createChildPath(filterArgs.value<std::string>(k_FeaturePhasesArrayName_Key));

  const auto& cellPhases = dataStructure.getDataAs<Int32Array>(pCellPhasesArrayPathValue)->getDataStoreRef();
  const auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  const auto& featureIds = featureIdsArray.getDataStoreRef();
  auto& featurePhases = dataStructure.getDataAs<Int32Array>(pFeaturePhasesArrayPathValue)->getDataStoreRef();

  // Validate the featurePhases array is the proper size
  auto validateResults = ValidateNumFeaturesInArray(dataStructure, pFeaturePhasesArrayPathValue, featureIdsArray);
  if(validateResults.invalid())
  {
    return validateResults;
  }

  usize totalPoints = featureIds.getNumberOfTuples();
  std::map<int32, int32> featureMap;
  std::map<int32, int32> warningMap;

  for(usize i = 0; i < totalPoints; i++)
  {
    if(shouldCancel)
    {
      return {};
    }

    int32 gnum = featureIds[i];
    featureMap.insert({gnum, cellPhases[i]});

    int32 curPhaseVal = featureMap[gnum];
    if(curPhaseVal != cellPhases[i])
    {
      auto [iter, insertSuccess] = warningMap.insert({gnum, 1});
      if(!insertSuccess)
      {
        iter->second += 1;
      }
    }
    featurePhases[gnum] = cellPhases[i];
  }

  Result<> result;
  if(!warningMap.empty())
  {
    result.warnings().push_back(Warning{-500, "Elements from some features did not all have the same phase ID. The last phase ID copied into each feature will be used."});
    for(auto&& [key, value] : warningMap)
    {
      result.warnings().push_back(Warning{-500, fmt::format("Phase Feature {} created {} warnings.", key, value)});
    }
  }

  return result;
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
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
