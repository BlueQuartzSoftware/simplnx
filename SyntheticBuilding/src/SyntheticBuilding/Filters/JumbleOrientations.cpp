#include "JumbleOrientations.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string JumbleOrientations::name() const
{
  return FilterTraits<JumbleOrientations>::name.str();
}

//------------------------------------------------------------------------------
std::string JumbleOrientations::className() const
{
  return FilterTraits<JumbleOrientations>::className;
}

//------------------------------------------------------------------------------
Uuid JumbleOrientations::uuid() const
{
  return FilterTraits<JumbleOrientations>::uuid;
}

//------------------------------------------------------------------------------
std::string JumbleOrientations::humanName() const
{
  return "Jumble Orientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> JumbleOrientations::defaultTags() const
{
  return {"#Synthetic Building", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters JumbleOrientations::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Average Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellEulerAnglesArrayName_Key, "Euler Angles", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_AvgQuatsArrayName_Key, "Average Quaternions", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer JumbleOrientations::clone() const
{
  return std::make_unique<JumbleOrientations>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult JumbleOrientations::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCellEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayName_Key);
  auto pAvgQuatsArrayNameValue = filterArgs.value<DataPath>(k_AvgQuatsArrayName_Key);

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
  auto action = std::make_unique<JumbleOrientationsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> JumbleOrientations::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCellEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayName_Key);
  auto pAvgQuatsArrayNameValue = filterArgs.value<DataPath>(k_AvgQuatsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
