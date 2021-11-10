#include "FindFeatureNeighborCAxisMisalignments.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeatureNeighborCAxisMisalignments::name() const
{
  return FilterTraits<FindFeatureNeighborCAxisMisalignments>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeatureNeighborCAxisMisalignments::className() const
{
  return FilterTraits<FindFeatureNeighborCAxisMisalignments>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeatureNeighborCAxisMisalignments::uuid() const
{
  return FilterTraits<FindFeatureNeighborCAxisMisalignments>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeatureNeighborCAxisMisalignments::humanName() const
{
  return "Find Feature Neighbor C-Axis Misalignments";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeatureNeighborCAxisMisalignments::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindFeatureNeighborCAxisMisalignments::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindAvgMisals_Key, "Find Average Misalignment Per Feature", "", false));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NeighborListArrayPath_Key, "Neighbor List", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CAxisMisalignmentListArrayName_Key, "C-Axis Misalignment List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_AvgCAxisMisalignmentsArrayName_Key, "Avgerage C-Axis Misalignments", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_FindAvgMisals_Key, k_AvgCAxisMisalignmentsArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeatureNeighborCAxisMisalignments::clone() const
{
  return std::make_unique<FindFeatureNeighborCAxisMisalignments>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindFeatureNeighborCAxisMisalignments::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFindAvgMisalsValue = filterArgs.value<bool>(k_FindAvgMisals_Key);
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCAxisMisalignmentListArrayNameValue = filterArgs.value<DataPath>(k_CAxisMisalignmentListArrayName_Key);
  auto pAvgCAxisMisalignmentsArrayNameValue = filterArgs.value<DataPath>(k_AvgCAxisMisalignmentsArrayName_Key);

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
  auto action = std::make_unique<FindFeatureNeighborCAxisMisalignmentsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindFeatureNeighborCAxisMisalignments::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFindAvgMisalsValue = filterArgs.value<bool>(k_FindAvgMisals_Key);
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCAxisMisalignmentListArrayNameValue = filterArgs.value<DataPath>(k_CAxisMisalignmentListArrayName_Key);
  auto pAvgCAxisMisalignmentsArrayNameValue = filterArgs.value<DataPath>(k_AvgCAxisMisalignmentsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
