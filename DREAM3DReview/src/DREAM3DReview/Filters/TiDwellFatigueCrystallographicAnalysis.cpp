#include "TiDwellFatigueCrystallographicAnalysis.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string TiDwellFatigueCrystallographicAnalysis::name() const
{
  return FilterTraits<TiDwellFatigueCrystallographicAnalysis>::name.str();
}

//------------------------------------------------------------------------------
std::string TiDwellFatigueCrystallographicAnalysis::className() const
{
  return FilterTraits<TiDwellFatigueCrystallographicAnalysis>::className;
}

//------------------------------------------------------------------------------
Uuid TiDwellFatigueCrystallographicAnalysis::uuid() const
{
  return FilterTraits<TiDwellFatigueCrystallographicAnalysis>::uuid;
}

//------------------------------------------------------------------------------
std::string TiDwellFatigueCrystallographicAnalysis::humanName() const
{
  return "Ti Dwell Fatigue Crystallographic Analysis";
}

//------------------------------------------------------------------------------
std::vector<std::string> TiDwellFatigueCrystallographicAnalysis::defaultTags() const
{
  return {"#Unsupported", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters TiDwellFatigueCrystallographicAnalysis::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_AlphaGlobPhasePresent_Key, "Alpha Glob Phase Present", "", false));
  params.insert(std::make_unique<Int32Parameter>(k_AlphaGlobPhase_Key, "Alpha Glob Phase Number", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_MTRPhase_Key, "Microtextured Region Phase Number", "", 1234356));
  params.insert(std::make_unique<Float32Parameter>(k_LatticeParameterA_Key, "Lattice Parameter A", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_LatticeParameterC_Key, "Lattice Parameter C", "", 1.23345f));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_StressAxis_Key, "Stress Axis", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<Int32Parameter>(k_SubsurfaceDistance_Key, "Subsurface Feature Distance to Consider (Microns)", "", 1234356));
  params.insert(std::make_unique<Float32Parameter>(k_ConsiderationFraction_Key, "Fraction of Features to Consider", "", 1.23345f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_DoNotAssumeInitiatorPresence_Key, "Do Not Assume Initiator Presence", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_InitiatorLowerThreshold_Key, "Initiator Lower Threshold (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_InitiatorUpperThreshold_Key, "Initiator Upper Threshold (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_HardFeatureLowerThreshold_Key, "Hard Feature Lower Threshold (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_HardFeatureUpperThreshold_Key, "Hard Feature Upper Threshold (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_SoftFeatureLowerThreshold_Key, "Soft Feature Lower Threshold (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_SoftFeatureUpperThreshold_Key, "Soft Feature Upper Threshold (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "FeatureIds", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Average Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NeighborListArrayPath_Key, "Neighbor List", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellParentIdsArrayName_Key, "Parent Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellFeatureAttributeMatrixName_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SelectedFeaturesArrayName_Key, "Selected Features", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_InitiatorsArrayName_Key, "Initiators", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_HardFeaturesArrayName_Key, "Hard Features", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SoftFeaturesArrayName_Key, "Soft Features", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_HardSoftGroupsArrayName_Key, "Hard-Soft Groups", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureParentIdsArrayName_Key, "Parent Ids", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_AlphaGlobPhasePresent_Key, k_AlphaGlobPhase_Key, true);
  params.linkParameters(k_DoNotAssumeInitiatorPresence_Key, k_InitiatorLowerThreshold_Key, true);
  params.linkParameters(k_DoNotAssumeInitiatorPresence_Key, k_InitiatorUpperThreshold_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer TiDwellFatigueCrystallographicAnalysis::clone() const
{
  return std::make_unique<TiDwellFatigueCrystallographicAnalysis>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult TiDwellFatigueCrystallographicAnalysis::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pAlphaGlobPhasePresentValue = filterArgs.value<bool>(k_AlphaGlobPhasePresent_Key);
  auto pAlphaGlobPhaseValue = filterArgs.value<int32>(k_AlphaGlobPhase_Key);
  auto pMTRPhaseValue = filterArgs.value<int32>(k_MTRPhase_Key);
  auto pLatticeParameterAValue = filterArgs.value<float32>(k_LatticeParameterA_Key);
  auto pLatticeParameterCValue = filterArgs.value<float32>(k_LatticeParameterC_Key);
  auto pStressAxisValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_StressAxis_Key);
  auto pSubsurfaceDistanceValue = filterArgs.value<int32>(k_SubsurfaceDistance_Key);
  auto pConsiderationFractionValue = filterArgs.value<float32>(k_ConsiderationFraction_Key);
  auto pDoNotAssumeInitiatorPresenceValue = filterArgs.value<bool>(k_DoNotAssumeInitiatorPresence_Key);
  auto pInitiatorLowerThresholdValue = filterArgs.value<float32>(k_InitiatorLowerThreshold_Key);
  auto pInitiatorUpperThresholdValue = filterArgs.value<float32>(k_InitiatorUpperThreshold_Key);
  auto pHardFeatureLowerThresholdValue = filterArgs.value<float32>(k_HardFeatureLowerThreshold_Key);
  auto pHardFeatureUpperThresholdValue = filterArgs.value<float32>(k_HardFeatureUpperThreshold_Key);
  auto pSoftFeatureLowerThresholdValue = filterArgs.value<float32>(k_SoftFeatureLowerThreshold_Key);
  auto pSoftFeatureUpperThresholdValue = filterArgs.value<float32>(k_SoftFeatureUpperThreshold_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellParentIdsArrayNameValue = filterArgs.value<DataPath>(k_CellParentIdsArrayName_Key);
  auto pNewCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  auto pSelectedFeaturesArrayNameValue = filterArgs.value<DataPath>(k_SelectedFeaturesArrayName_Key);
  auto pInitiatorsArrayNameValue = filterArgs.value<DataPath>(k_InitiatorsArrayName_Key);
  auto pHardFeaturesArrayNameValue = filterArgs.value<DataPath>(k_HardFeaturesArrayName_Key);
  auto pSoftFeaturesArrayNameValue = filterArgs.value<DataPath>(k_SoftFeaturesArrayName_Key);
  auto pHardSoftGroupsArrayNameValue = filterArgs.value<DataPath>(k_HardSoftGroupsArrayName_Key);
  auto pFeatureParentIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);

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
  auto action = std::make_unique<TiDwellFatigueCrystallographicAnalysisAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> TiDwellFatigueCrystallographicAnalysis::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pAlphaGlobPhasePresentValue = filterArgs.value<bool>(k_AlphaGlobPhasePresent_Key);
  auto pAlphaGlobPhaseValue = filterArgs.value<int32>(k_AlphaGlobPhase_Key);
  auto pMTRPhaseValue = filterArgs.value<int32>(k_MTRPhase_Key);
  auto pLatticeParameterAValue = filterArgs.value<float32>(k_LatticeParameterA_Key);
  auto pLatticeParameterCValue = filterArgs.value<float32>(k_LatticeParameterC_Key);
  auto pStressAxisValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_StressAxis_Key);
  auto pSubsurfaceDistanceValue = filterArgs.value<int32>(k_SubsurfaceDistance_Key);
  auto pConsiderationFractionValue = filterArgs.value<float32>(k_ConsiderationFraction_Key);
  auto pDoNotAssumeInitiatorPresenceValue = filterArgs.value<bool>(k_DoNotAssumeInitiatorPresence_Key);
  auto pInitiatorLowerThresholdValue = filterArgs.value<float32>(k_InitiatorLowerThreshold_Key);
  auto pInitiatorUpperThresholdValue = filterArgs.value<float32>(k_InitiatorUpperThreshold_Key);
  auto pHardFeatureLowerThresholdValue = filterArgs.value<float32>(k_HardFeatureLowerThreshold_Key);
  auto pHardFeatureUpperThresholdValue = filterArgs.value<float32>(k_HardFeatureUpperThreshold_Key);
  auto pSoftFeatureLowerThresholdValue = filterArgs.value<float32>(k_SoftFeatureLowerThreshold_Key);
  auto pSoftFeatureUpperThresholdValue = filterArgs.value<float32>(k_SoftFeatureUpperThreshold_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellParentIdsArrayNameValue = filterArgs.value<DataPath>(k_CellParentIdsArrayName_Key);
  auto pNewCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  auto pSelectedFeaturesArrayNameValue = filterArgs.value<DataPath>(k_SelectedFeaturesArrayName_Key);
  auto pInitiatorsArrayNameValue = filterArgs.value<DataPath>(k_InitiatorsArrayName_Key);
  auto pHardFeaturesArrayNameValue = filterArgs.value<DataPath>(k_HardFeaturesArrayName_Key);
  auto pSoftFeaturesArrayNameValue = filterArgs.value<DataPath>(k_SoftFeaturesArrayName_Key);
  auto pHardSoftGroupsArrayNameValue = filterArgs.value<DataPath>(k_HardSoftGroupsArrayName_Key);
  auto pFeatureParentIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
