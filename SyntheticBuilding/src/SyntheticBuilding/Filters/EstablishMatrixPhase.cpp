#include "EstablishMatrixPhase.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string EstablishMatrixPhase::name() const
{
  return FilterTraits<EstablishMatrixPhase>::name.str();
}

//------------------------------------------------------------------------------
std::string EstablishMatrixPhase::className() const
{
  return FilterTraits<EstablishMatrixPhase>::className;
}

//------------------------------------------------------------------------------
Uuid EstablishMatrixPhase::uuid() const
{
  return FilterTraits<EstablishMatrixPhase>::uuid;
}

//------------------------------------------------------------------------------
std::string EstablishMatrixPhase::humanName() const
{
  return "Establish Matrix Phase";
}

//------------------------------------------------------------------------------
std::vector<std::string> EstablishMatrixPhase::defaultTags() const
{
  return {"#Synthetic Building", "#Packing"};
}

//------------------------------------------------------------------------------
Parameters EstablishMatrixPhase::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_OutputCellAttributeMatrixPath_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellPhasesArrayName_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputCellFeatureAttributeMatrixName_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeaturePhasesArrayName_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputStatsArrayPath_Key, "Statistics", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputPhaseTypesArrayPath_Key, "Phase Types", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputPhaseNamesArrayPath_Key, "Phase Names", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputCellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NumFeaturesArrayName_Key, "Number of Features", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer EstablishMatrixPhase::clone() const
{
  return std::make_unique<EstablishMatrixPhase>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult EstablishMatrixPhase::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pOutputCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_OutputCellAttributeMatrixPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pCellPhasesArrayNameValue = filterArgs.value<DataPath>(k_CellPhasesArrayName_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pOutputCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellFeatureAttributeMatrixName_Key);
  auto pFeaturePhasesArrayNameValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayName_Key);
  auto pInputStatsArrayPathValue = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  auto pInputPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  auto pInputPhaseNamesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseNamesArrayPath_Key);
  auto pOutputCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellEnsembleAttributeMatrixName_Key);
  auto pNumFeaturesArrayNameValue = filterArgs.value<DataPath>(k_NumFeaturesArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<EstablishMatrixPhaseAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> EstablishMatrixPhase::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pOutputCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_OutputCellAttributeMatrixPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pCellPhasesArrayNameValue = filterArgs.value<DataPath>(k_CellPhasesArrayName_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pOutputCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellFeatureAttributeMatrixName_Key);
  auto pFeaturePhasesArrayNameValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayName_Key);
  auto pInputStatsArrayPathValue = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  auto pInputPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  auto pInputPhaseNamesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseNamesArrayPath_Key);
  auto pOutputCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputCellEnsembleAttributeMatrixName_Key);
  auto pNumFeaturesArrayNameValue = filterArgs.value<DataPath>(k_NumFeaturesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
