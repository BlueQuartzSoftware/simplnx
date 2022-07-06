#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  PackPrimaryPhasesInputValues inputValues;

  inputValues.PeriodicBoundaries = filterArgs.value<bool>(k_PeriodicBoundaries_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.OutputCellAttributeMatrixPath = filterArgs.value<DataPath>(k_OutputCellAttributeMatrixPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.InputStatsArrayPath = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  inputValues.InputPhaseTypesArrayPath = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  inputValues.InputPhaseNamesArrayPath = filterArgs.value<DataPath>(k_InputPhaseNamesArrayPath_Key);
  inputValues.InputShapeTypesArrayPath = filterArgs.value<DataPath>(k_InputShapeTypesArrayPath_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  inputValues.CellPhasesArrayName = filterArgs.value<DataPath>(k_CellPhasesArrayName_Key);
  inputValues.OutputCellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_OutputCellFeatureAttributeMatrixName_Key);
  inputValues.FeaturePhasesArrayName = filterArgs.value<DataPath>(k_FeaturePhasesArrayName_Key);
  inputValues.OutputCellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_OutputCellEnsembleAttributeMatrixName_Key);
  inputValues.NumFeaturesArrayName = filterArgs.value<DataPath>(k_NumFeaturesArrayName_Key);
  inputValues.FeatureGeneration = filterArgs.value<ChoicesParameter::ValueType>(k_FeatureGeneration_Key);
  inputValues.FeatureInputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_FeatureInputFile_Key);
  inputValues.SaveGeometricDescriptions = filterArgs.value<ChoicesParameter::ValueType>(k_SaveGeometricDescriptions_Key);
  inputValues.NewAttributeMatrixPath = filterArgs.value<DataPath>(k_NewAttributeMatrixPath_Key);
  inputValues.SelectedAttributeMatrixPath = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);

  return PackPrimaryPhases(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SYNTHETICBUILDING_EXPORT PackPrimaryPhasesInputValues
{
  bool PeriodicBoundaries;
  bool UseMask;
  DataPath OutputCellAttributeMatrixPath;
  DataPath MaskArrayPath;
  DataPath InputStatsArrayPath;
  DataPath InputPhaseTypesArrayPath;
  DataPath InputPhaseNamesArrayPath;
  DataPath InputShapeTypesArrayPath;
  DataPath FeatureIdsArrayName;
  DataPath CellPhasesArrayName;
  DataPath OutputCellFeatureAttributeMatrixName;
  DataPath FeaturePhasesArrayName;
  DataPath OutputCellEnsembleAttributeMatrixName;
  DataPath NumFeaturesArrayName;
  ChoicesParameter::ValueType FeatureGeneration;
  FileSystemPathParameter::ValueType FeatureInputFile;
  ChoicesParameter::ValueType SaveGeometricDescriptions;
  DataPath NewAttributeMatrixPath;
  DataPath SelectedAttributeMatrixPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SYNTHETICBUILDING_EXPORT PackPrimaryPhases
{
public:
  PackPrimaryPhases(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PackPrimaryPhasesInputValues* inputValues);
  ~PackPrimaryPhases() noexcept;

  PackPrimaryPhases(const PackPrimaryPhases&) = delete;
  PackPrimaryPhases(PackPrimaryPhases&&) noexcept = delete;
  PackPrimaryPhases& operator=(const PackPrimaryPhases&) = delete;
  PackPrimaryPhases& operator=(PackPrimaryPhases&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const PackPrimaryPhasesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
