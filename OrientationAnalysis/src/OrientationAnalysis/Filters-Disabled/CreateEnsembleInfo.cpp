#include "CreateEnsembleInfo.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/EnsembleInfoFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateEnsembleInfo::name() const
{
  return FilterTraits<CreateEnsembleInfo>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateEnsembleInfo::className() const
{
  return FilterTraits<CreateEnsembleInfo>::className;
}

//------------------------------------------------------------------------------
Uuid CreateEnsembleInfo::uuid() const
{
  return FilterTraits<CreateEnsembleInfo>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateEnsembleInfo::humanName() const
{
  return "Create Ensemble Info";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateEnsembleInfo::defaultTags() const
{
  return {"#Processing", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters CreateEnsembleInfo::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  /*[x]*/ params.insert(std::make_unique<EnsembleInfoFilterParameter>(k_Ensemble_Key, "Created Ensemble Info", "", {}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellEnsembleAttributeMatrixName_Key, "Ensemble Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CrystalStructuresArrayName_Key, "Crystal Structures", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PhaseTypesArrayName_Key, "Phase Types", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PhaseNamesArrayName_Key, "Phase Names", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateEnsembleInfo::clone() const
{
  return std::make_unique<CreateEnsembleInfo>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateEnsembleInfo::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pEnsembleValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_Ensemble_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);
  auto pPhaseTypesArrayNameValue = filterArgs.value<DataPath>(k_PhaseTypesArrayName_Key);
  auto pPhaseNamesArrayNameValue = filterArgs.value<DataPath>(k_PhaseNamesArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateEnsembleInfoAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CreateEnsembleInfo::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pEnsembleValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_Ensemble_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pCrystalStructuresArrayNameValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);
  auto pPhaseTypesArrayNameValue = filterArgs.value<DataPath>(k_PhaseTypesArrayName_Key);
  auto pPhaseNamesArrayNameValue = filterArgs.value<DataPath>(k_PhaseNamesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
