#include "FindFeaturePhasesFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

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
  return {"#Generic", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindFeaturePhasesFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "", DataPath{}));

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
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);

  complex::Result<OutputActions> resultOutputActions;

  const IDataArray& featureIds = dataStructure.getDataRefAs<IDataArray>(pFeatureIdsArrayPathValue);

  {
    auto createFeaturePhasesAction = std::make_unique<CreateArrayAction>(NumericType::int32, std::vector<usize>{featureIds.getNumberOfTuples()}, std::vector<usize>{1}, pFeaturePhasesArrayPathValue);
    resultOutputActions.value().actions.push_back(std::move(createFeaturePhasesAction));
  }

  std::vector<PreflightValue> preflightUpdatedValues;

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindFeaturePhasesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);

  const Int32Array& cellPhases = dataStructure.getDataRefAs<Int32Array>(pCellPhasesArrayPathValue);
  const Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  Int32Array& featurePhases = dataStructure.getDataRefAs<Int32Array>(pFeaturePhasesArrayPathValue);

  // Resize the featurePhases array to the proper size
  usize featureIdsMaxIdx = std::distance(featureIds.begin(), std::max_element(featureIds.cbegin(), featureIds.cend()));
  usize maxValue = featureIds[featureIdsMaxIdx];

  DataStore<int32>& featurePhasesStore = featurePhases.getIDataStoreRefAs<DataStore<int32>>();
  featurePhasesStore.reshapeTuples(std::vector<usize>{maxValue + 1});

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
    if(featureMap.find(gnum) == featureMap.end())
    {
      featureMap.emplace(gnum, cellPhases[i]);
    }

    int32 curPhaseVal = featureMap[gnum];
    if(curPhaseVal != cellPhases[i])
    {
      if(warningMap.find(gnum) == warningMap.end())
      {
        warningMap[gnum] = 1;
      }
      else
      {
        warningMap[gnum]++;
      }
    }
    featurePhases[gnum] = cellPhases[i];
  }

  Result<> result;
  if(!warningMap.empty())
  {
    result.warnings().push_back(Warning{-500, "Elements from some features did not all have the same phase ID. The last phase ID copied into each feature will be used."});
    for(std::map<int32_t, int32_t>::iterator iter = warningMap.begin(); iter != warningMap.end(); iter++)
    {
      result.warnings().push_back(Warning{-500, fmt::format("Phase Feature {} created {} warnings.", iter->first, iter->second)});
    }
  }

  return result;
}
} // namespace complex
