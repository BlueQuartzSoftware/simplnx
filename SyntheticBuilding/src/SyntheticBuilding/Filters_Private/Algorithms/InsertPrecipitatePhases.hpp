#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  InsertPrecipitatePhasesInputValues inputValues;

  inputValues.PeriodicBoundaries = filterArgs.value<bool>(k_PeriodicBoundaries_Key);
  inputValues.MatchRDF = filterArgs.value<bool>(k_MatchRDF_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.BoundaryCellsArrayPath = filterArgs.value<DataPath>(k_BoundaryCellsArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.InputStatsArrayPath = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  inputValues.InputPhaseTypesArrayPath = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  inputValues.InputShapeTypesArrayPath = filterArgs.value<DataPath>(k_InputShapeTypesArrayPath_Key);
  inputValues.NumFeaturesArrayPath = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);
  inputValues.FeatureGeneration = filterArgs.value<ChoicesParameter::ValueType>(k_FeatureGeneration_Key);
  inputValues.PrecipInputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_PrecipInputFile_Key);
  inputValues.SaveGeometricDescriptions = filterArgs.value<ChoicesParameter::ValueType>(k_SaveGeometricDescriptions_Key);
  inputValues.NewAttributeMatrixPath = filterArgs.value<DataPath>(k_NewAttributeMatrixPath_Key);
  inputValues.SelectedAttributeMatrixPath = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);

  return InsertPrecipitatePhases(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SYNTHETICBUILDING_EXPORT InsertPrecipitatePhasesInputValues
{
  bool PeriodicBoundaries;
  bool MatchRDF;
  bool UseMask;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath BoundaryCellsArrayPath;
  DataPath MaskArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath InputStatsArrayPath;
  DataPath InputPhaseTypesArrayPath;
  DataPath InputShapeTypesArrayPath;
  DataPath NumFeaturesArrayPath;
  ChoicesParameter::ValueType FeatureGeneration;
  FileSystemPathParameter::ValueType PrecipInputFile;
  ChoicesParameter::ValueType SaveGeometricDescriptions;
  DataPath NewAttributeMatrixPath;
  DataPath SelectedAttributeMatrixPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SYNTHETICBUILDING_EXPORT InsertPrecipitatePhases
{
public:
  InsertPrecipitatePhases(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InsertPrecipitatePhasesInputValues* inputValues);
  ~InsertPrecipitatePhases() noexcept;

  InsertPrecipitatePhases(const InsertPrecipitatePhases&) = delete;
  InsertPrecipitatePhases(InsertPrecipitatePhases&&) noexcept = delete;
  InsertPrecipitatePhases& operator=(const InsertPrecipitatePhases&) = delete;
  InsertPrecipitatePhases& operator=(InsertPrecipitatePhases&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const InsertPrecipitatePhasesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
