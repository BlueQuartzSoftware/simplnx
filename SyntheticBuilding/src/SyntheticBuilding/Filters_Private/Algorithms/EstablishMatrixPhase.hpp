#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  EstablishMatrixPhaseInputValues inputValues;

  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.OutputCellAttributeMatrixPath = filterArgs.value<DataPath>(k_OutputCellAttributeMatrixPath_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  inputValues.CellPhasesArrayName = filterArgs.value<DataPath>(k_CellPhasesArrayName_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.OutputCellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_OutputCellFeatureAttributeMatrixName_Key);
  inputValues.FeaturePhasesArrayName = filterArgs.value<DataPath>(k_FeaturePhasesArrayName_Key);
  inputValues.InputStatsArrayPath = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  inputValues.InputPhaseTypesArrayPath = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  inputValues.InputPhaseNamesArrayPath = filterArgs.value<DataPath>(k_InputPhaseNamesArrayPath_Key);
  inputValues.OutputCellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_OutputCellEnsembleAttributeMatrixName_Key);
  inputValues.NumFeaturesArrayName = filterArgs.value<DataPath>(k_NumFeaturesArrayName_Key);

  return EstablishMatrixPhase(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SYNTHETICBUILDING_EXPORT EstablishMatrixPhaseInputValues
{
  bool UseMask;
  DataPath OutputCellAttributeMatrixPath;
  DataPath FeatureIdsArrayName;
  DataPath CellPhasesArrayName;
  DataPath MaskArrayPath;
  DataPath OutputCellFeatureAttributeMatrixName;
  DataPath FeaturePhasesArrayName;
  DataPath InputStatsArrayPath;
  DataPath InputPhaseTypesArrayPath;
  DataPath InputPhaseNamesArrayPath;
  DataPath OutputCellEnsembleAttributeMatrixName;
  DataPath NumFeaturesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SYNTHETICBUILDING_EXPORT EstablishMatrixPhase
{
public:
  EstablishMatrixPhase(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EstablishMatrixPhaseInputValues* inputValues);
  ~EstablishMatrixPhase() noexcept;

  EstablishMatrixPhase(const EstablishMatrixPhase&) = delete;
  EstablishMatrixPhase(EstablishMatrixPhase&&) noexcept = delete;
  EstablishMatrixPhase& operator=(const EstablishMatrixPhase&) = delete;
  EstablishMatrixPhase& operator=(EstablishMatrixPhase&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const EstablishMatrixPhaseInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
