#include "StatsGeneratorFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/StatsGeneratorFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string StatsGeneratorFilter::name() const
{
  return FilterTraits<StatsGeneratorFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string StatsGeneratorFilter::className() const
{
  return FilterTraits<StatsGeneratorFilter>::className;
}

//------------------------------------------------------------------------------
Uuid StatsGeneratorFilter::uuid() const
{
  return FilterTraits<StatsGeneratorFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string StatsGeneratorFilter::humanName() const
{
  return "StatsGenerator";
}

//------------------------------------------------------------------------------
std::vector<std::string> StatsGeneratorFilter::defaultTags() const
{
  return {"#Synthetic Building", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters StatsGeneratorFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<StatsGeneratorFilterParameter>(k_StatsGenerator_Key, "StatsGenerator", "", {}));
  params.insertSeparator(Parameters::Separator{"Created Data Container"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_StatsGeneratorDataContainerName_Key, "Statistics Data Container Name", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Created Ensemble AttributeMatrix"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix Name", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Created Ensemble Arrays"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_StatsDataArrayName_Key, "Statistics Array Name", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CrystalStructuresArrayName_Key, "Crystal Structures Array Name", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PhaseTypesArrayName_Key, "Phase Types Array Name", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PhaseNamesArrayName_Key, "Phase Names Array Name", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer StatsGeneratorFilter::clone() const
{
  return std::make_unique<StatsGeneratorFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult StatsGeneratorFilter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pStatsGeneratorValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_StatsGenerator_Key);
  auto pStatsGeneratorDataContainerNameValue = filterArgs.value<DataPath>(k_StatsGeneratorDataContainerName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pStatsDataArrayNameValue = filterArgs.value<DataPath>(k_StatsDataArrayName_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);
  auto pPhaseTypesArrayNameValue = filterArgs.value<DataPath>(k_PhaseTypesArrayName_Key);
  auto pPhaseNamesArrayNameValue = filterArgs.value<DataPath>(k_PhaseNamesArrayName_Key);

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
  auto action = std::make_unique<StatsGeneratorFilterAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> StatsGeneratorFilter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pStatsGeneratorValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_StatsGenerator_Key);
  auto pStatsGeneratorDataContainerNameValue = filterArgs.value<DataPath>(k_StatsGeneratorDataContainerName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pStatsDataArrayNameValue = filterArgs.value<DataPath>(k_StatsDataArrayName_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);
  auto pPhaseTypesArrayNameValue = filterArgs.value<DataPath>(k_PhaseTypesArrayName_Key);
  auto pPhaseNamesArrayNameValue = filterArgs.value<DataPath>(k_PhaseNamesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
