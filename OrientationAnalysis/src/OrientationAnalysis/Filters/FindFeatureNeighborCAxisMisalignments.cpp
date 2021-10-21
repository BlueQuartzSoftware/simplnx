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
Result<OutputActions> FindFeatureNeighborCAxisMisalignments::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFindAvgMisalsValue = filterArgs.value<bool>(k_FindAvgMisals_Key);
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCAxisMisalignmentListArrayNameValue = filterArgs.value<DataPath>(k_CAxisMisalignmentListArrayName_Key);
  auto pAvgCAxisMisalignmentsArrayNameValue = filterArgs.value<DataPath>(k_AvgCAxisMisalignmentsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindFeatureNeighborCAxisMisalignmentsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindFeatureNeighborCAxisMisalignments::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
