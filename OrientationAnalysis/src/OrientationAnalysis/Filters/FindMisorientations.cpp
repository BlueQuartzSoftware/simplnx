#include "FindMisorientations.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
std::string FindMisorientations::name() const
{
  return FilterTraits<FindMisorientations>::name.str();
}

std::string FindMisorientations::className() const
{
  return FilterTraits<FindMisorientations>::className;
}

Uuid FindMisorientations::uuid() const
{
  return FilterTraits<FindMisorientations>::uuid;
}

std::string FindMisorientations::humanName() const
{
  return "Find Feature Neighbor Misorientations";
}

Parameters FindMisorientations::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindAvgMisors_Key, "Find Average Misorientation Per Feature", "", false));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NeighborListArrayPath_Key, "Neighbor List", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_MisorientationListArrayName_Key, "Misorientation List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_AvgMisorientationsArrayName_Key, "Average Misorientations", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_FindAvgMisors_Key, k_AvgMisorientationsArrayName_Key, true);

  return params;
}

IFilter::UniquePointer FindMisorientations::clone() const
{
  return std::make_unique<FindMisorientations>();
}

Result<OutputActions> FindMisorientations::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFindAvgMisorsValue = filterArgs.value<bool>(k_FindAvgMisors_Key);
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pMisorientationListArrayNameValue = filterArgs.value<DataPath>(k_MisorientationListArrayName_Key);
  auto pAvgMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_AvgMisorientationsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindMisorientationsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FindMisorientations::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFindAvgMisorsValue = filterArgs.value<bool>(k_FindAvgMisors_Key);
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pMisorientationListArrayNameValue = filterArgs.value<DataPath>(k_MisorientationListArrayName_Key);
  auto pAvgMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_AvgMisorientationsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
