#include "FindFeaturePhasesFilter.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeaturePhasesFilter::name() const
{
  return FilterTraits<FindFeaturePhasesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeaturePhasesFilter::className() const
{
  return FilterTraits<FindFeaturePhasesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeaturePhasesFilter::uuid() const
{
  return FilterTraits<FindFeaturePhasesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeaturePhasesFilter::humanName() const
{
  return "Find Feature Phases";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeaturePhasesFilter::defaultTags() const
{
  return {className(), "Generic", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindFeaturePhasesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Element belongs", DataPath({"Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Feature Ids", "Specifies to which Feature each Element belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeaturesAttributeMatrixPath_Key, "Cell Feature Attribute Matrix", "The AttributeMatrix that stores the feature data for the input **Feature Ids**.", DataPath({"CellFeatureData"})));

  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "The name of the feature attribute matrix in which to store the found feature phases array", "Phases"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeaturePhasesFilter::clone() const
{
  return std::make_unique<FindFeaturePhasesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindFeaturePhasesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellFeatureAMPathValue = filterArgs.value<DataPath>(k_CellFeaturesAttributeMatrixPath_Key);
  auto pFeaturePhasesArrayPathValue = pCellFeatureAMPathValue.createChildPath(filterArgs.value<std::string>(k_FeaturePhasesArrayPath_Key));

  complex::Result<OutputActions> resultOutputActions;

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
Result<> FindFeaturePhasesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellFeatureAMPathValue = filterArgs.value<DataPath>(k_CellFeaturesAttributeMatrixPath_Key);
  auto pFeaturePhasesArrayPathValue = pCellFeatureAMPathValue.createChildPath(filterArgs.value<std::string>(k_FeaturePhasesArrayPath_Key));

  const Int32Array& cellPhases = dataStructure.getDataRefAs<Int32Array>(pCellPhasesArrayPathValue);
  const Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  Int32Array& featurePhases = dataStructure.getDataRefAs<Int32Array>(pFeaturePhasesArrayPathValue);

  // Validate the featurePhases array is the proper size
  auto validateResults = ValidateNumFeaturesInArray(dataStructure, pFeaturePhasesArrayPathValue, featureIds);
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
} // namespace complex
