#include "NeighborOrientationCorrelation.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string NeighborOrientationCorrelation::name() const
{
  return FilterTraits<NeighborOrientationCorrelation>::name.str();
}

//------------------------------------------------------------------------------
std::string NeighborOrientationCorrelation::className() const
{
  return FilterTraits<NeighborOrientationCorrelation>::className;
}

//------------------------------------------------------------------------------
Uuid NeighborOrientationCorrelation::uuid() const
{
  return FilterTraits<NeighborOrientationCorrelation>::uuid;
}

//------------------------------------------------------------------------------
std::string NeighborOrientationCorrelation::humanName() const
{
  return "Neighbor Orientation Correlation";
}

//------------------------------------------------------------------------------
std::vector<std::string> NeighborOrientationCorrelation::defaultTags() const
{
  return {"#Processing", "#Cleanup"};
}

//------------------------------------------------------------------------------
Parameters NeighborOrientationCorrelation::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_MinConfidence_Key, "Minimum Confidence Index", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<Int32Parameter>(k_Level_Key, "Cleanup Level", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ConfidenceIndexArrayPath_Key, "Confidence Index", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer NeighborOrientationCorrelation::clone() const
{
  return std::make_unique<NeighborOrientationCorrelation>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult NeighborOrientationCorrelation::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMinConfidenceValue = filterArgs.value<float32>(k_MinConfidence_Key);
  auto pMisorientationToleranceValue = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pLevelValue = filterArgs.value<int32>(k_Level_Key);
  auto pConfidenceIndexArrayPathValue = filterArgs.value<DataPath>(k_ConfidenceIndexArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

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
  auto action = std::make_unique<NeighborOrientationCorrelationAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> NeighborOrientationCorrelation::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMinConfidenceValue = filterArgs.value<float32>(k_MinConfidence_Key);
  auto pMisorientationToleranceValue = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pLevelValue = filterArgs.value<int32>(k_Level_Key);
  auto pConfidenceIndexArrayPathValue = filterArgs.value<DataPath>(k_ConfidenceIndexArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
