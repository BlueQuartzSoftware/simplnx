#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindGBCDMetricBasedInputValues inputValues;

  inputValues.PhaseOfInterest = filterArgs.value<int32>(k_PhaseOfInterest_Key);
  inputValues.MisorientationRotation = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MisorientationRotation_Key);
  inputValues.ChosenLimitDists = filterArgs.value<ChoicesParameter::ValueType>(k_ChosenLimitDists_Key);
  inputValues.NumSamplPts = filterArgs.value<int32>(k_NumSamplPts_Key);
  inputValues.ExcludeTripleLines = filterArgs.value<bool>(k_ExcludeTripleLines_Key);
  inputValues.DistOutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_DistOutputFile_Key);
  inputValues.ErrOutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_ErrOutputFile_Key);
  inputValues.SaveRelativeErr = filterArgs.value<bool>(k_SaveRelativeErr_Key);
  inputValues.NodeTypesArrayPath = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.SurfaceMeshFaceNormalsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  inputValues.SurfaceMeshFaceAreasArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceAreasArrayPath_Key);
  inputValues.SurfaceMeshFeatureFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceLabelsArrayPath_Key);
  inputValues.FeatureEulerAnglesArrayPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  return FindGBCDMetricBased(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindGBCDMetricBasedInputValues
{
  int32 PhaseOfInterest;
  VectorFloat32Parameter::ValueType MisorientationRotation;
  ChoicesParameter::ValueType ChosenLimitDists;
  int32 NumSamplPts;
  bool ExcludeTripleLines;
  FileSystemPathParameter::ValueType DistOutputFile;
  FileSystemPathParameter::ValueType ErrOutputFile;
  bool SaveRelativeErr;
  DataPath NodeTypesArrayPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFaceNormalsArrayPath;
  DataPath SurfaceMeshFaceAreasArrayPath;
  DataPath SurfaceMeshFeatureFaceLabelsArrayPath;
  DataPath FeatureEulerAnglesArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindGBCDMetricBased
{
public:
  FindGBCDMetricBased(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindGBCDMetricBasedInputValues* inputValues);
  ~FindGBCDMetricBased() noexcept;

  FindGBCDMetricBased(const FindGBCDMetricBased&) = delete;
  FindGBCDMetricBased(FindGBCDMetricBased&&) noexcept = delete;
  FindGBCDMetricBased& operator=(const FindGBCDMetricBased&) = delete;
  FindGBCDMetricBased& operator=(FindGBCDMetricBased&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindGBCDMetricBasedInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
