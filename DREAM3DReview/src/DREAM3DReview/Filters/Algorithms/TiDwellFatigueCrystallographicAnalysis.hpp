#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  TiDwellFatigueCrystallographicAnalysisInputValues inputValues;

  inputValues.AlphaGlobPhasePresent = filterArgs.value<bool>(k_AlphaGlobPhasePresent_Key);
  inputValues.AlphaGlobPhase = filterArgs.value<int32>(k_AlphaGlobPhase_Key);
  inputValues.MTRPhase = filterArgs.value<int32>(k_MTRPhase_Key);
  inputValues.LatticeParameterA = filterArgs.value<float32>(k_LatticeParameterA_Key);
  inputValues.LatticeParameterC = filterArgs.value<float32>(k_LatticeParameterC_Key);
  inputValues.StressAxis = filterArgs.value<VectorFloat32Parameter::ValueType>(k_StressAxis_Key);
  inputValues.SubsurfaceDistance = filterArgs.value<int32>(k_SubsurfaceDistance_Key);
  inputValues.ConsiderationFraction = filterArgs.value<float32>(k_ConsiderationFraction_Key);
  inputValues.DoNotAssumeInitiatorPresence = filterArgs.value<bool>(k_DoNotAssumeInitiatorPresence_Key);
  inputValues.InitiatorLowerThreshold = filterArgs.value<float32>(k_InitiatorLowerThreshold_Key);
  inputValues.InitiatorUpperThreshold = filterArgs.value<float32>(k_InitiatorUpperThreshold_Key);
  inputValues.HardFeatureLowerThreshold = filterArgs.value<float32>(k_HardFeatureLowerThreshold_Key);
  inputValues.HardFeatureUpperThreshold = filterArgs.value<float32>(k_HardFeatureUpperThreshold_Key);
  inputValues.SoftFeatureLowerThreshold = filterArgs.value<float32>(k_SoftFeatureLowerThreshold_Key);
  inputValues.SoftFeatureUpperThreshold = filterArgs.value<float32>(k_SoftFeatureUpperThreshold_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellFeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  inputValues.FeatureEulerAnglesArrayPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.NeighborListArrayPath = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.CellParentIdsArrayName = filterArgs.value<DataPath>(k_CellParentIdsArrayName_Key);
  inputValues.NewCellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  inputValues.SelectedFeaturesArrayName = filterArgs.value<DataPath>(k_SelectedFeaturesArrayName_Key);
  inputValues.InitiatorsArrayName = filterArgs.value<DataPath>(k_InitiatorsArrayName_Key);
  inputValues.HardFeaturesArrayName = filterArgs.value<DataPath>(k_HardFeaturesArrayName_Key);
  inputValues.SoftFeaturesArrayName = filterArgs.value<DataPath>(k_SoftFeaturesArrayName_Key);
  inputValues.HardSoftGroupsArrayName = filterArgs.value<DataPath>(k_HardSoftGroupsArrayName_Key);
  inputValues.FeatureParentIdsArrayName = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);

  return TiDwellFatigueCrystallographicAnalysis(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT TiDwellFatigueCrystallographicAnalysisInputValues
{
  bool AlphaGlobPhasePresent;
  int32 AlphaGlobPhase;
  int32 MTRPhase;
  float32 LatticeParameterA;
  float32 LatticeParameterC;
  VectorFloat32Parameter::ValueType StressAxis;
  int32 SubsurfaceDistance;
  float32 ConsiderationFraction;
  bool DoNotAssumeInitiatorPresence;
  float32 InitiatorLowerThreshold;
  float32 InitiatorUpperThreshold;
  float32 HardFeatureLowerThreshold;
  float32 HardFeatureUpperThreshold;
  float32 SoftFeatureLowerThreshold;
  float32 SoftFeatureUpperThreshold;
  DataPath DataContainerName;
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureAttributeMatrixPath;
  DataPath FeatureEulerAnglesArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath NeighborListArrayPath;
  DataPath CentroidsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath CellParentIdsArrayName;
  DataPath NewCellFeatureAttributeMatrixName;
  DataPath SelectedFeaturesArrayName;
  DataPath InitiatorsArrayName;
  DataPath HardFeaturesArrayName;
  DataPath SoftFeaturesArrayName;
  DataPath HardSoftGroupsArrayName;
  DataPath FeatureParentIdsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT TiDwellFatigueCrystallographicAnalysis
{
public:
  TiDwellFatigueCrystallographicAnalysis(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                         TiDwellFatigueCrystallographicAnalysisInputValues* inputValues);
  ~TiDwellFatigueCrystallographicAnalysis() noexcept;

  TiDwellFatigueCrystallographicAnalysis(const TiDwellFatigueCrystallographicAnalysis&) = delete;
  TiDwellFatigueCrystallographicAnalysis(TiDwellFatigueCrystallographicAnalysis&&) noexcept = delete;
  TiDwellFatigueCrystallographicAnalysis& operator=(const TiDwellFatigueCrystallographicAnalysis&) = delete;
  TiDwellFatigueCrystallographicAnalysis& operator=(TiDwellFatigueCrystallographicAnalysis&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const TiDwellFatigueCrystallographicAnalysisInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
