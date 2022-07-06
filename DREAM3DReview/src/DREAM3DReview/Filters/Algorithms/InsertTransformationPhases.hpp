#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  InsertTransformationPhasesInputValues inputValues;

  inputValues.ParentPhase = filterArgs.value<int32>(k_ParentPhase_Key);
  inputValues.TransCrystalStruct = filterArgs.value<ChoicesParameter::ValueType>(k_TransCrystalStruct_Key);
  inputValues.TransformationPhaseMisorientation = filterArgs.value<float32>(k_TransformationPhaseMisorientation_Key);
  inputValues.DefineHabitPlane = filterArgs.value<bool>(k_DefineHabitPlane_Key);
  inputValues.TransformationPhaseHabitPlane = filterArgs.value<VectorFloat32Parameter::ValueType>(k_TransformationPhaseHabitPlane_Key);
  inputValues.UseAllVariants = filterArgs.value<bool>(k_UseAllVariants_Key);
  inputValues.CoherentFrac = filterArgs.value<float32>(k_CoherentFrac_Key);
  inputValues.TransformationPhaseThickness = filterArgs.value<float32>(k_TransformationPhaseThickness_Key);
  inputValues.NumTransformationPhasesPerFeature = filterArgs.value<int32>(k_NumTransformationPhasesPerFeature_Key);
  inputValues.PeninsulaFrac = filterArgs.value<float32>(k_PeninsulaFrac_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  inputValues.FeatureEulerAnglesArrayPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.EquivalentDiametersArrayPath = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.StatsGenCellEnsembleAttributeMatrixPath = filterArgs.value<DataPath>(k_StatsGenCellEnsembleAttributeMatrixPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.PhaseTypesArrayPath = filterArgs.value<DataPath>(k_PhaseTypesArrayPath_Key);
  inputValues.ShapeTypesArrayPath = filterArgs.value<DataPath>(k_ShapeTypesArrayPath_Key);
  inputValues.NumFeaturesArrayPath = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);
  inputValues.FeatureParentIdsArrayName = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);
  inputValues.NumFeaturesPerParentArrayPath = filterArgs.value<DataPath>(k_NumFeaturesPerParentArrayPath_Key);

  return InsertTransformationPhases(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT InsertTransformationPhasesInputValues
{
  int32 ParentPhase;
  ChoicesParameter::ValueType TransCrystalStruct;
  float32 TransformationPhaseMisorientation;
  bool DefineHabitPlane;
  VectorFloat32Parameter::ValueType TransformationPhaseHabitPlane;
  bool UseAllVariants;
  float32 CoherentFrac;
  float32 TransformationPhaseThickness;
  int32 NumTransformationPhasesPerFeature;
  float32 PeninsulaFrac;
  DataPath FeatureIdsArrayPath;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CellFeatureAttributeMatrixName;
  DataPath FeatureEulerAnglesArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath CentroidsArrayPath;
  DataPath EquivalentDiametersArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath StatsGenCellEnsembleAttributeMatrixPath;
  DataPath CrystalStructuresArrayPath;
  DataPath PhaseTypesArrayPath;
  DataPath ShapeTypesArrayPath;
  DataPath NumFeaturesArrayPath;
  DataPath FeatureParentIdsArrayName;
  DataPath NumFeaturesPerParentArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT InsertTransformationPhases
{
public:
  InsertTransformationPhases(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InsertTransformationPhasesInputValues* inputValues);
  ~InsertTransformationPhases() noexcept;

  InsertTransformationPhases(const InsertTransformationPhases&) = delete;
  InsertTransformationPhases(InsertTransformationPhases&&) noexcept = delete;
  InsertTransformationPhases& operator=(const InsertTransformationPhases&) = delete;
  InsertTransformationPhases& operator=(InsertTransformationPhases&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const InsertTransformationPhasesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
