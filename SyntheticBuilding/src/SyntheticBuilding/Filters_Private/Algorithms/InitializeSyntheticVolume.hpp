#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  InitializeSyntheticVolumeInputValues inputValues;

  inputValues.EstimateNumberOfFeatures = filterArgs.value<bool>(k_EstimateNumberOfFeatures_Key);
  inputValues.EstimatedPrimaryFeatures = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_EstimatedPrimaryFeatures_Key);
  inputValues.Geometry Selection = filterArgs.value<{}>(k_Geometry Selection_Key);
  inputValues.GeometryDataContainer = filterArgs.value<DataPath>(k_GeometryDataContainer_Key);
  inputValues.InputStatsArrayPath = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  inputValues.InputPhaseTypesArrayPath = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.GeometrySelection = filterArgs.value<ChoicesParameter::ValueType>(k_GeometrySelection_Key);
  inputValues.Dimensions = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.LengthUnit = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  inputValues.BoxDimensions = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_BoxDimensions_Key);

  return InitializeSyntheticVolume(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SYNTHETICBUILDING_EXPORT InitializeSyntheticVolumeInputValues
{
  bool EstimateNumberOfFeatures;
  <<<NOT_IMPLEMENTED>>> EstimatedPrimaryFeatures;
  {
  }
  Geometry Selection;
  DataPath GeometryDataContainer;
  DataPath InputStatsArrayPath;
  DataPath InputPhaseTypesArrayPath;
  DataPath DataContainerName;
  DataPath CellAttributeMatrixName;
  ChoicesParameter::ValueType GeometrySelection;
  VectorInt32Parameter::ValueType Dimensions;
  VectorFloat32Parameter::ValueType Spacing;
  VectorFloat32Parameter::ValueType Origin;
  ChoicesParameter::ValueType LengthUnit;
  <<<NOT_IMPLEMENTED>>> BoxDimensions;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SYNTHETICBUILDING_EXPORT InitializeSyntheticVolume
{
public:
  InitializeSyntheticVolume(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InitializeSyntheticVolumeInputValues* inputValues);
  ~InitializeSyntheticVolume() noexcept;

  InitializeSyntheticVolume(const InitializeSyntheticVolume&) = delete;
  InitializeSyntheticVolume(InitializeSyntheticVolume&&) noexcept = delete;
  InitializeSyntheticVolume& operator=(const InitializeSyntheticVolume&) = delete;
  InitializeSyntheticVolume& operator=(InitializeSyntheticVolume&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const InitializeSyntheticVolumeInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
