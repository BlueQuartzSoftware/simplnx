#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindTwinBoundarySchmidFactorsInputValues inputValues;

  inputValues.LoadingDir = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LoadingDir_Key);
  inputValues.WriteFile = filterArgs.value<bool>(k_WriteFile_Key);
  inputValues.TwinBoundarySchmidFactorsFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_TwinBoundarySchmidFactorsFile_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.SurfaceMeshFaceNormalsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  inputValues.SurfaceMeshTwinBoundaryArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshTwinBoundaryArrayPath_Key);
  inputValues.SurfaceMeshTwinBoundarySchmidFactorsArrayName = filterArgs.value<DataPath>(k_SurfaceMeshTwinBoundarySchmidFactorsArrayName_Key);

  return FindTwinBoundarySchmidFactors(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindTwinBoundarySchmidFactorsInputValues
{
  VectorFloat32Parameter::ValueType LoadingDir;
  bool WriteFile;
  FileSystemPathParameter::ValueType TwinBoundarySchmidFactorsFile;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFaceNormalsArrayPath;
  DataPath SurfaceMeshTwinBoundaryArrayPath;
  DataPath SurfaceMeshTwinBoundarySchmidFactorsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindTwinBoundarySchmidFactors
{
public:
  FindTwinBoundarySchmidFactors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindTwinBoundarySchmidFactorsInputValues* inputValues);
  ~FindTwinBoundarySchmidFactors() noexcept;

  FindTwinBoundarySchmidFactors(const FindTwinBoundarySchmidFactors&) = delete;
  FindTwinBoundarySchmidFactors(FindTwinBoundarySchmidFactors&&) noexcept = delete;
  FindTwinBoundarySchmidFactors& operator=(const FindTwinBoundarySchmidFactors&) = delete;
  FindTwinBoundarySchmidFactors& operator=(FindTwinBoundarySchmidFactors&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindTwinBoundarySchmidFactorsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
