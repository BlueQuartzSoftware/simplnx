#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  EstablishFoamMorphologyInputValues inputValues;

  inputValues.PeriodicBoundaries = filterArgs.value<bool>(k_PeriodicBoundaries_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.InputStatsArrayPath = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  inputValues.InputPhaseTypesArrayPath = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  inputValues.InputPhaseNamesArrayPath = filterArgs.value<DataPath>(k_InputPhaseNamesArrayPath_Key);
  inputValues.InputShapeTypesArrayPath = filterArgs.value<DataPath>(k_InputShapeTypesArrayPath_Key);
  inputValues.InputCellFeatureIdsArrayPath = filterArgs.value<DataPath>(k_InputCellFeatureIdsArrayPath_Key);
  inputValues.OutputCellEnsembleAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_OutputCellEnsembleAttributeMatrixName_Key);
  inputValues.NumFeaturesArrayName = filterArgs.value<StringParameter::ValueType>(k_NumFeaturesArrayName_Key);
  inputValues.OutputCellFeatureAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_OutputCellFeatureAttributeMatrixName_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<StringParameter::ValueType>(k_FeatureIdsArrayName_Key);
  inputValues.MaskArrayName = filterArgs.value<StringParameter::ValueType>(k_MaskArrayName_Key);
  inputValues.CellPhasesArrayName = filterArgs.value<StringParameter::ValueType>(k_CellPhasesArrayName_Key);
  inputValues.QPEuclideanDistancesArrayName = filterArgs.value<StringParameter::ValueType>(k_QPEuclideanDistancesArrayName_Key);
  inputValues.TJEuclideanDistancesArrayName = filterArgs.value<StringParameter::ValueType>(k_TJEuclideanDistancesArrayName_Key);
  inputValues.GBEuclideanDistancesArrayName = filterArgs.value<StringParameter::ValueType>(k_GBEuclideanDistancesArrayName_Key);
  inputValues.WriteGoalAttributes = filterArgs.value<bool>(k_WriteGoalAttributes_Key);
  inputValues.CsvOutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_CsvOutputFile_Key);
  inputValues.HaveFeatures = filterArgs.value<ChoicesParameter::ValueType>(k_HaveFeatures_Key);
  inputValues.MinStrutThickness = filterArgs.value<float64>(k_MinStrutThickness_Key);
  inputValues.StrutThicknessVariability = filterArgs.value<float32>(k_StrutThicknessVariability_Key);
  inputValues.StrutShapeVariability = filterArgs.value<float32>(k_StrutShapeVariability_Key);

  return EstablishFoamMorphology(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT EstablishFoamMorphologyInputValues
{
  bool PeriodicBoundaries;
  DataPath DataContainerName;
  DataPath InputStatsArrayPath;
  DataPath InputPhaseTypesArrayPath;
  DataPath InputPhaseNamesArrayPath;
  DataPath InputShapeTypesArrayPath;
  DataPath InputCellFeatureIdsArrayPath;
  StringParameter::ValueType OutputCellEnsembleAttributeMatrixName;
  StringParameter::ValueType NumFeaturesArrayName;
  StringParameter::ValueType OutputCellFeatureAttributeMatrixName;
  StringParameter::ValueType FeatureIdsArrayName;
  StringParameter::ValueType MaskArrayName;
  StringParameter::ValueType CellPhasesArrayName;
  StringParameter::ValueType QPEuclideanDistancesArrayName;
  StringParameter::ValueType TJEuclideanDistancesArrayName;
  StringParameter::ValueType GBEuclideanDistancesArrayName;
  bool WriteGoalAttributes;
  FileSystemPathParameter::ValueType CsvOutputFile;
  ChoicesParameter::ValueType HaveFeatures;
  float64 MinStrutThickness;
  float32 StrutThicknessVariability;
  float32 StrutShapeVariability;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT EstablishFoamMorphology
{
public:
  EstablishFoamMorphology(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EstablishFoamMorphologyInputValues* inputValues);
  ~EstablishFoamMorphology() noexcept;

  EstablishFoamMorphology(const EstablishFoamMorphology&) = delete;
  EstablishFoamMorphology(EstablishFoamMorphology&&) noexcept = delete;
  EstablishFoamMorphology& operator=(const EstablishFoamMorphology&) = delete;
  EstablishFoamMorphology& operator=(EstablishFoamMorphology&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const EstablishFoamMorphologyInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
