#include "FindEuclideanDistMap.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindEuclideanDistMap::name() const
{
  return FilterTraits<FindEuclideanDistMap>::name.str();
}

//------------------------------------------------------------------------------
std::string FindEuclideanDistMap::className() const
{
  return FilterTraits<FindEuclideanDistMap>::className;
}

//------------------------------------------------------------------------------
Uuid FindEuclideanDistMap::uuid() const
{
  return FilterTraits<FindEuclideanDistMap>::uuid;
}

//------------------------------------------------------------------------------
std::string FindEuclideanDistMap::humanName() const
{
  return "Find Euclidean Distance Map";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindEuclideanDistMap::defaultTags() const
{
  return {"#Statistics", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindEuclideanDistMap::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_CalcManhattanDist_Key, "Calculate Manhattan Distance", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_DoBoundaries_Key, "Calculate Distance to Boundaries", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_DoTripleLines_Key, "Calculate Distance to Triple Lines", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_DoQuadPoints_Key, "Calculate Distance to Quadruple Points", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveNearestNeighbors_Key, "Store the Nearest Boundary Cells", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_GBDistancesArrayName_Key, "Boundary Distances", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_TJDistancesArrayName_Key, "Triple Line Distances", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_QPDistancesArrayName_Key, "Quadruple Point Distances", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NearestNeighborsArrayName_Key, "Nearest Neighbors", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_DoBoundaries_Key, k_GBDistancesArrayName_Key, true);
  params.linkParameters(k_DoTripleLines_Key, k_TJDistancesArrayName_Key, true);
  params.linkParameters(k_DoQuadPoints_Key, k_QPDistancesArrayName_Key, true);
  params.linkParameters(k_SaveNearestNeighbors_Key, k_NearestNeighborsArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindEuclideanDistMap::clone() const
{
  return std::make_unique<FindEuclideanDistMap>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindEuclideanDistMap::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pCalcManhattanDistValue = filterArgs.value<bool>(k_CalcManhattanDist_Key);
  auto pDoBoundariesValue = filterArgs.value<bool>(k_DoBoundaries_Key);
  auto pDoTripleLinesValue = filterArgs.value<bool>(k_DoTripleLines_Key);
  auto pDoQuadPointsValue = filterArgs.value<bool>(k_DoQuadPoints_Key);
  auto pSaveNearestNeighborsValue = filterArgs.value<bool>(k_SaveNearestNeighbors_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pGBDistancesArrayNameValue = filterArgs.value<DataPath>(k_GBDistancesArrayName_Key);
  auto pTJDistancesArrayNameValue = filterArgs.value<DataPath>(k_TJDistancesArrayName_Key);
  auto pQPDistancesArrayNameValue = filterArgs.value<DataPath>(k_QPDistancesArrayName_Key);
  auto pNearestNeighborsArrayNameValue = filterArgs.value<DataPath>(k_NearestNeighborsArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindEuclideanDistMapAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindEuclideanDistMap::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCalcManhattanDistValue = filterArgs.value<bool>(k_CalcManhattanDist_Key);
  auto pDoBoundariesValue = filterArgs.value<bool>(k_DoBoundaries_Key);
  auto pDoTripleLinesValue = filterArgs.value<bool>(k_DoTripleLines_Key);
  auto pDoQuadPointsValue = filterArgs.value<bool>(k_DoQuadPoints_Key);
  auto pSaveNearestNeighborsValue = filterArgs.value<bool>(k_SaveNearestNeighbors_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pGBDistancesArrayNameValue = filterArgs.value<DataPath>(k_GBDistancesArrayName_Key);
  auto pTJDistancesArrayNameValue = filterArgs.value<DataPath>(k_TJDistancesArrayName_Key);
  auto pQPDistancesArrayNameValue = filterArgs.value<DataPath>(k_QPDistancesArrayName_Key);
  auto pNearestNeighborsArrayNameValue = filterArgs.value<DataPath>(k_NearestNeighborsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
