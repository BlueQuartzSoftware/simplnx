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
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "FeatureIds", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Average Euler Angles", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath({"FeatureData", "Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NeighborListArrayPath_Key, "Neighbor List", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "", DataPath({"FeatureData", "Centroids"}), ArraySelectionParameter::AllowedTypes{DataType::float32}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath({"Ensemble Data", "CrystalStructures"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}));
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
IFilter::PreflightResult TiDwellFatigueCrystallographicAnalysis::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                               const std::atomic_bool& shouldCancel) const
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

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> TiDwellFatigueCrystallographicAnalysis::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
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
