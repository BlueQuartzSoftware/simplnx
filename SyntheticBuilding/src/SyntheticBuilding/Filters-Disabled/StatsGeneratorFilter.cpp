#include "StatsGeneratorFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/StatsGeneratorFilterParameter.hpp"

using namespace complex;

namespace complex
{
std::string StatsGeneratorFilter::name() const
{
  return FilterTraits<StatsGeneratorFilter>::name.str();
}

Uuid StatsGeneratorFilter::uuid() const
{
  return FilterTraits<StatsGeneratorFilter>::uuid;
}

std::string StatsGeneratorFilter::humanName() const
{
  return "StatsGenerator";
}

Parameters StatsGeneratorFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<StatsGeneratorFilterParameter>(k_StatsGenerator_Key, "StatsGenerator", "", {}));
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

IFilter::UniquePointer StatsGeneratorFilter::clone() const
{
  return std::make_unique<StatsGeneratorFilter>();
}

Result<OutputActions> StatsGeneratorFilter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pStatsGeneratorValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_StatsGenerator_Key);
  auto pStatsGeneratorDataContainerNameValue = filterArgs.value<DataPath>(k_StatsGeneratorDataContainerName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pStatsDataArrayNameValue = filterArgs.value<DataPath>(k_StatsDataArrayName_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);
  auto pPhaseTypesArrayNameValue = filterArgs.value<DataPath>(k_PhaseTypesArrayName_Key);
  auto pPhaseNamesArrayNameValue = filterArgs.value<DataPath>(k_PhaseNamesArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<StatsGeneratorFilterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> StatsGeneratorFilter::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
