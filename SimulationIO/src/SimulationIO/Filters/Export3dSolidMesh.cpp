#include "Export3dSolidMesh.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string Export3dSolidMesh::name() const
{
  return FilterTraits<Export3dSolidMesh>::name.str();
}

Uuid Export3dSolidMesh::uuid() const
{
  return FilterTraits<Export3dSolidMesh>::uuid;
}

std::string Export3dSolidMesh::humanName() const
{
  return "Export 3d Solid Mesh";
}

Parameters Export3dSolidMesh::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_MeshingPackage_Key, "Meshing package", "", 0, ChoicesParameter::Choices{"TetGen", "Netgen", "Gmsh"}));
  params.insert(std::make_unique<FileSystemPathParameter>(k_outputPath_Key, "Path", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<FileSystemPathParameter>(k_PackageLocation_Key, "Package Location", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_NetgenSTLFileName_Key, "STL File Prefix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_GmshSTLFileName_Key, "STL File Prefix", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureCentroidArrayPath_Key, "Feature Centroids", "", DataPath{}));
  params.insert(std::make_unique<ChoicesParameter>(k_MeshFileFormat_Key, "Mesh File Format", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertSeparator(Parameters::Separator{"Mesh Quality Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RefineMesh_Key, "Refine Mesh (q)", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_MaxRadiusEdgeRatio_Key, "Maximum Radius-Edge Ratio", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_MinDihedralAngle_Key, "Minimum Dihedral Angle", "", 1.23345f));
  params.insert(std::make_unique<Int32Parameter>(k_OptimizationLevel_Key, "Optimization Level (O)", "", 1234356));
  params.insert(std::make_unique<ChoicesParameter>(k_MeshSize_Key, "Mesh Size", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertSeparator(Parameters::Separator{"Topology Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_LimitTetrahedraVolume_Key, "Limit Tetrahedra Volume (a)", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_MaxTetrahedraVolume_Key, "Maximum Tetrahedron Volume", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Holes in the Mesh"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_IncludeHolesUsingPhaseID_Key, "Include Holes Using PhaseID", "", false));
  params.insert(std::make_unique<Int32Parameter>(k_PhaseID_Key, "PhaseID", "", 1234356));
  params.insertSeparator(Parameters::Separator{""});
  params.insert(std::make_unique<StringParameter>(k_TetDataContainerName_Key, "Data Container Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_MeshingPackage_Key, k_SurfaceMeshFaceLabelsArrayPath_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_FeaturePhasesArrayPath_Key, 1);
  params.linkParameters(k_MeshingPackage_Key, k_FeatureCentroidArrayPath_Key, 2);
  params.linkParameters(k_MeshingPackage_Key, k_RefineMesh_Key, 3);
  params.linkParameters(k_MeshingPackage_Key, k_MaxRadiusEdgeRatio_Key, 4);
  params.linkParameters(k_MeshingPackage_Key, k_MinDihedralAngle_Key, 5);
  params.linkParameters(k_MeshingPackage_Key, k_OptimizationLevel_Key, 6);
  params.linkParameters(k_MeshingPackage_Key, k_LimitTetrahedraVolume_Key, 7);
  params.linkParameters(k_MeshingPackage_Key, k_MaxTetrahedraVolume_Key, 8);
  params.linkParameters(k_MeshingPackage_Key, k_IncludeHolesUsingPhaseID_Key, 9);
  params.linkParameters(k_MeshingPackage_Key, k_PhaseID_Key, 10);
  params.linkParameters(k_MeshingPackage_Key, k_TetDataContainerName_Key, 11);
  params.linkParameters(k_MeshingPackage_Key, k_VertexAttributeMatrixName_Key, 12);
  params.linkParameters(k_MeshingPackage_Key, k_CellAttributeMatrixName_Key, 13);
  params.linkParameters(k_MeshingPackage_Key, k_GmshSTLFileName_Key, 14);
  params.linkParameters(k_MeshingPackage_Key, k_NetgenSTLFileName_Key, 15);
  params.linkParameters(k_MeshingPackage_Key, k_MeshSize_Key, 16);
  params.linkParameters(k_MeshingPackage_Key, k_MeshFileFormat_Key, 17);
  params.linkParameters(k_RefineMesh_Key, k_MaxRadiusEdgeRatio_Key, true);
  params.linkParameters(k_RefineMesh_Key, k_MinDihedralAngle_Key, true);
  params.linkParameters(k_LimitTetrahedraVolume_Key, k_MaxTetrahedraVolume_Key, true);
  params.linkParameters(k_IncludeHolesUsingPhaseID_Key, k_PhaseID_Key, true);

  return params;
}

IFilter::UniquePointer Export3dSolidMesh::clone() const
{
  return std::make_unique<Export3dSolidMesh>();
}

Result<OutputActions> Export3dSolidMesh::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMeshingPackageValue = filterArgs.value<ChoicesParameter::ValueType>(k_MeshingPackage_Key);
  auto poutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_outputPath_Key);
  auto pPackageLocationValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_PackageLocation_Key);
  auto pNetgenSTLFileNameValue = filterArgs.value<StringParameter::ValueType>(k_NetgenSTLFileName_Key);
  auto pGmshSTLFileNameValue = filterArgs.value<StringParameter::ValueType>(k_GmshSTLFileName_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pFeatureCentroidArrayPathValue = filterArgs.value<DataPath>(k_FeatureCentroidArrayPath_Key);
  auto pMeshFileFormatValue = filterArgs.value<ChoicesParameter::ValueType>(k_MeshFileFormat_Key);
  auto pRefineMeshValue = filterArgs.value<bool>(k_RefineMesh_Key);
  auto pMaxRadiusEdgeRatioValue = filterArgs.value<float32>(k_MaxRadiusEdgeRatio_Key);
  auto pMinDihedralAngleValue = filterArgs.value<float32>(k_MinDihedralAngle_Key);
  auto pOptimizationLevelValue = filterArgs.value<int32>(k_OptimizationLevel_Key);
  auto pMeshSizeValue = filterArgs.value<ChoicesParameter::ValueType>(k_MeshSize_Key);
  auto pLimitTetrahedraVolumeValue = filterArgs.value<bool>(k_LimitTetrahedraVolume_Key);
  auto pMaxTetrahedraVolumeValue = filterArgs.value<float32>(k_MaxTetrahedraVolume_Key);
  auto pIncludeHolesUsingPhaseIDValue = filterArgs.value<bool>(k_IncludeHolesUsingPhaseID_Key);
  auto pPhaseIDValue = filterArgs.value<int32>(k_PhaseID_Key);
  auto pTetDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_TetDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<Export3dSolidMeshAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> Export3dSolidMesh::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMeshingPackageValue = filterArgs.value<ChoicesParameter::ValueType>(k_MeshingPackage_Key);
  auto poutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_outputPath_Key);
  auto pPackageLocationValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_PackageLocation_Key);
  auto pNetgenSTLFileNameValue = filterArgs.value<StringParameter::ValueType>(k_NetgenSTLFileName_Key);
  auto pGmshSTLFileNameValue = filterArgs.value<StringParameter::ValueType>(k_GmshSTLFileName_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pFeatureCentroidArrayPathValue = filterArgs.value<DataPath>(k_FeatureCentroidArrayPath_Key);
  auto pMeshFileFormatValue = filterArgs.value<ChoicesParameter::ValueType>(k_MeshFileFormat_Key);
  auto pRefineMeshValue = filterArgs.value<bool>(k_RefineMesh_Key);
  auto pMaxRadiusEdgeRatioValue = filterArgs.value<float32>(k_MaxRadiusEdgeRatio_Key);
  auto pMinDihedralAngleValue = filterArgs.value<float32>(k_MinDihedralAngle_Key);
  auto pOptimizationLevelValue = filterArgs.value<int32>(k_OptimizationLevel_Key);
  auto pMeshSizeValue = filterArgs.value<ChoicesParameter::ValueType>(k_MeshSize_Key);
  auto pLimitTetrahedraVolumeValue = filterArgs.value<bool>(k_LimitTetrahedraVolume_Key);
  auto pMaxTetrahedraVolumeValue = filterArgs.value<float32>(k_MaxTetrahedraVolume_Key);
  auto pIncludeHolesUsingPhaseIDValue = filterArgs.value<bool>(k_IncludeHolesUsingPhaseID_Key);
  auto pPhaseIDValue = filterArgs.value<int32>(k_PhaseID_Key);
  auto pTetDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_TetDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
