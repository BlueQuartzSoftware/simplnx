#include "FindSlipTransmissionMetrics.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindSlipTransmissionMetrics::name() const
{
  return FilterTraits<FindSlipTransmissionMetrics>::name.str();
}

//------------------------------------------------------------------------------
std::string FindSlipTransmissionMetrics::className() const
{
  return FilterTraits<FindSlipTransmissionMetrics>::className;
}

//------------------------------------------------------------------------------
Uuid FindSlipTransmissionMetrics::uuid() const
{
  return FilterTraits<FindSlipTransmissionMetrics>::uuid;
}

//------------------------------------------------------------------------------
std::string FindSlipTransmissionMetrics::humanName() const
{
  return "Find Neighbor Slip Transmission Metrics";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindSlipTransmissionMetrics::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindSlipTransmissionMetrics::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NeighborListArrayPath_Key, "Neighbor List", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_F1ListArrayName_Key, "F1 List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_F1sptListArrayName_Key, "F1spt List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_F7ListArrayName_Key, "F7 List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_mPrimeListArrayName_Key, "mPrime List", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindSlipTransmissionMetrics::clone() const
{
  return std::make_unique<FindSlipTransmissionMetrics>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindSlipTransmissionMetrics::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pF1ListArrayNameValue = filterArgs.value<DataPath>(k_F1ListArrayName_Key);
  auto pF1sptListArrayNameValue = filterArgs.value<DataPath>(k_F1sptListArrayName_Key);
  auto pF7ListArrayNameValue = filterArgs.value<DataPath>(k_F7ListArrayName_Key);
  auto pmPrimeListArrayNameValue = filterArgs.value<DataPath>(k_mPrimeListArrayName_Key);

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
  auto action = std::make_unique<FindSlipTransmissionMetricsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindSlipTransmissionMetrics::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pF1ListArrayNameValue = filterArgs.value<DataPath>(k_F1ListArrayName_Key);
  auto pF1sptListArrayNameValue = filterArgs.value<DataPath>(k_F1sptListArrayName_Key);
  auto pF7ListArrayNameValue = filterArgs.value<DataPath>(k_F7ListArrayName_Key);
  auto pmPrimeListArrayNameValue = filterArgs.value<DataPath>(k_mPrimeListArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
