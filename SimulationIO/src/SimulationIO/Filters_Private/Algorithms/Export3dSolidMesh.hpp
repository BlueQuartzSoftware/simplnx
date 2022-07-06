#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  Export3dSolidMeshInputValues inputValues;

  inputValues.MeshingPackage = filterArgs.value<ChoicesParameter::ValueType>(k_MeshingPackage_Key);
  inputValues.outputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_outputPath_Key);
  inputValues.PackageLocation = filterArgs.value<FileSystemPathParameter::ValueType>(k_PackageLocation_Key);
  inputValues.NetgenSTLFileName = filterArgs.value<StringParameter::ValueType>(k_NetgenSTLFileName_Key);
  inputValues.GmshSTLFileName = filterArgs.value<StringParameter::ValueType>(k_GmshSTLFileName_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.FeatureEulerAnglesArrayPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.FeatureCentroidArrayPath = filterArgs.value<DataPath>(k_FeatureCentroidArrayPath_Key);
  inputValues.MeshFileFormat = filterArgs.value<ChoicesParameter::ValueType>(k_MeshFileFormat_Key);
  inputValues.RefineMesh = filterArgs.value<bool>(k_RefineMesh_Key);
  inputValues.MaxRadiusEdgeRatio = filterArgs.value<float32>(k_MaxRadiusEdgeRatio_Key);
  inputValues.MinDihedralAngle = filterArgs.value<float32>(k_MinDihedralAngle_Key);
  inputValues.OptimizationLevel = filterArgs.value<int32>(k_OptimizationLevel_Key);
  inputValues.MeshSize = filterArgs.value<ChoicesParameter::ValueType>(k_MeshSize_Key);
  inputValues.LimitTetrahedraVolume = filterArgs.value<bool>(k_LimitTetrahedraVolume_Key);
  inputValues.MaxTetrahedraVolume = filterArgs.value<float32>(k_MaxTetrahedraVolume_Key);
  inputValues.IncludeHolesUsingPhaseID = filterArgs.value<bool>(k_IncludeHolesUsingPhaseID_Key);
  inputValues.PhaseID = filterArgs.value<int32>(k_PhaseID_Key);
  inputValues.TetDataContainerName = filterArgs.value<StringParameter::ValueType>(k_TetDataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);

  return Export3dSolidMesh(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SIMULATIONIO_EXPORT Export3dSolidMeshInputValues
{
  ChoicesParameter::ValueType MeshingPackage;
  FileSystemPathParameter::ValueType outputPath;
  FileSystemPathParameter::ValueType PackageLocation;
  StringParameter::ValueType NetgenSTLFileName;
  StringParameter::ValueType GmshSTLFileName;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath FeatureEulerAnglesArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath FeatureCentroidArrayPath;
  ChoicesParameter::ValueType MeshFileFormat;
  bool RefineMesh;
  float32 MaxRadiusEdgeRatio;
  float32 MinDihedralAngle;
  int32 OptimizationLevel;
  ChoicesParameter::ValueType MeshSize;
  bool LimitTetrahedraVolume;
  float32 MaxTetrahedraVolume;
  bool IncludeHolesUsingPhaseID;
  int32 PhaseID;
  StringParameter::ValueType TetDataContainerName;
  StringParameter::ValueType VertexAttributeMatrixName;
  StringParameter::ValueType CellAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMULATIONIO_EXPORT Export3dSolidMesh
{
public:
  Export3dSolidMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, Export3dSolidMeshInputValues* inputValues);
  ~Export3dSolidMesh() noexcept;

  Export3dSolidMesh(const Export3dSolidMesh&) = delete;
  Export3dSolidMesh(Export3dSolidMesh&&) noexcept = delete;
  Export3dSolidMesh& operator=(const Export3dSolidMesh&) = delete;
  Export3dSolidMesh& operator=(Export3dSolidMesh&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const Export3dSolidMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
