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
//------------------------------------------------------------------------------
std::string Export3dSolidMesh::name() const
{
  return FilterTraits<Export3dSolidMesh>::name.str();
}

//------------------------------------------------------------------------------
std::string Export3dSolidMesh::className() const
{
  return FilterTraits<Export3dSolidMesh>::className;
}

//------------------------------------------------------------------------------
Uuid Export3dSolidMesh::uuid() const
{
  return FilterTraits<Export3dSolidMesh>::uuid;
}

//------------------------------------------------------------------------------
std::string Export3dSolidMesh::humanName() const
{
  return "Export 3d Solid Mesh";
}

//------------------------------------------------------------------------------
std::vector<std::string> Export3dSolidMesh::defaultTags() const
{
  return {"#Unsupported", "#SimulationIO"};
}

//------------------------------------------------------------------------------
Parameters Export3dSolidMesh::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_MeshingPackage_Key, "Meshing package", "", 0, ChoicesParameter::Choices{"TetGen", "Netgen", "Gmsh"}));
  params.insert(std::make_unique<FileSystemPathParameter>(k_outputPath_Key, "Path", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<FileSystemPathParameter>(k_PackageLocation_Key, "Package Location", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_NetgenSTLFileName_Key, "STL File Prefix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_GmshSTLFileName_Key, "STL File Prefix", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Euler Angles", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each cell belongs", DataPath({"FeatureData", "Phases"}), ArraySelectionParameter::AllowedTypes{complex::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureCentroidArrayPath_Key, "Feature Centroids", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
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
  params.linkParameters(k_MeshingPackage_Key, k_FeaturePhasesArrayPath_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_FeatureCentroidArrayPath_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_RefineMesh_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_MaxRadiusEdgeRatio_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_MinDihedralAngle_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_OptimizationLevel_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_LimitTetrahedraVolume_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_MaxTetrahedraVolume_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_IncludeHolesUsingPhaseID_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_PhaseID_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_TetDataContainerName_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_VertexAttributeMatrixName_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_CellAttributeMatrixName_Key, 0);
  params.linkParameters(k_MeshingPackage_Key, k_GmshSTLFileName_Key, 2);
  params.linkParameters(k_MeshingPackage_Key, k_NetgenSTLFileName_Key, 1);
  params.linkParameters(k_MeshingPackage_Key, k_MeshSize_Key, 1);
  params.linkParameters(k_MeshingPackage_Key, k_MeshFileFormat_Key, 2);
  params.linkParameters(k_RefineMesh_Key, k_MaxRadiusEdgeRatio_Key, true);
  params.linkParameters(k_RefineMesh_Key, k_MinDihedralAngle_Key, true);
  params.linkParameters(k_LimitTetrahedraVolume_Key, k_MaxTetrahedraVolume_Key, true);
  params.linkParameters(k_IncludeHolesUsingPhaseID_Key, k_PhaseID_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer Export3dSolidMesh::clone() const
{
  return std::make_unique<Export3dSolidMesh>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult Export3dSolidMesh::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
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

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> Export3dSolidMesh::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
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
