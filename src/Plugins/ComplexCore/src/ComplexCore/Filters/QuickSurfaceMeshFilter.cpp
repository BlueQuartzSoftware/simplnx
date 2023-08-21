#include "QuickSurfaceMeshFilter.hpp"

#include "ComplexCore/Filters/Algorithms/QuickSurfaceMesh.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CopyArrayInstanceAction.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/CreateGeometry2DAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string QuickSurfaceMeshFilter::name() const
{
  return FilterTraits<QuickSurfaceMeshFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string QuickSurfaceMeshFilter::className() const
{
  return FilterTraits<QuickSurfaceMeshFilter>::className;
}

//------------------------------------------------------------------------------
Uuid QuickSurfaceMeshFilter::uuid() const
{
  return FilterTraits<QuickSurfaceMeshFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string QuickSurfaceMeshFilter::humanName() const
{
  return "Quick Surface Mesh";
}

//------------------------------------------------------------------------------
std::vector<std::string> QuickSurfaceMeshFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Generation", "Create", "Triangle", "Geoemtry"};
}

//------------------------------------------------------------------------------
Parameters QuickSurfaceMeshFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<BoolParameter>(k_FixProblemVoxels_Key, "Attempt to Fix Problem Voxels", "See help page.", false));
  params.insert(std::make_unique<BoolParameter>(k_GenerateTripleLines_Key, "Generate Triple Lines", "Experimental feature. May not work.", false));

  params.insertSeparator(Parameters::Separator{"Cell Data"});

  params.insert(std::make_unique<GeometrySelectionParameter>(k_GridGeometryDataPath_Key, "Grid Geometry", "The complete path to the Grid Geometry from which to create a Triangle Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image, IGeometry::Type::RectGrid}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Feature Ids", "The complete path to the Array specifying which Feature each Cell belongs to", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(
      k_SelectedDataArrayPaths_Key, "Attribute Arrays to Transfer", "The paths to the Arrays specifying which Cell Attribute Arrays to transfer to the created Triangle Geometry",
      MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, complex::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Created Triangle Geometry"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_TriangleGeometryName_Key, "Created Triangle Geometry", "The name of the created Triangle Geometry", DataPath({"TriangleDataContainer"})));

  params.insertSeparator(Parameters::Separator{"Created Vertex Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexDataGroupName_Key, "Vertex Data [AttributeMatrix]",
                                                          "The complete path to the DataGroup where the Vertex Data of the Triangle Geometry will be created", INodeGeometry0D::k_VertexDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NodeTypesArrayName_Key, "NodeType", "The complete path to the Array specifying the type of node in the Triangle Geometry", "NodeTypes"));

  params.insertSeparator(Parameters::Separator{"Created Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceDataGroupName_Key, "Face Data [AttributeMatrix]",
                                                          "The complete path to the DataGroup where the Face Data of the Triangle Geometry will be created", INodeGeometry2D::k_FaceDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceLabelsArrayName_Key, "Face Labels",
                                                          "The complete path to the Array specifying which Features are on either side of each Face in the Triangle Geometry", "FaceLabels"));
  params.insertSeparator(Parameters::Separator{"Created Face Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceFeatureAttributeMatrixName_Key, "Face Feature Data [AttributeMatrix]",
                                                          "The complete path to the DataGroup where the Feature Data will be stored.", INodeGeometry1D::k_FaceFeatureAttributeMatrix));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer QuickSurfaceMeshFilter::clone() const
{
  return std::make_unique<QuickSurfaceMeshFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult QuickSurfaceMeshFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto pGenerateTripleLines = filterArgs.value<bool>(k_GenerateTripleLines_Key);
  auto pFixProblemVoxelsValue = filterArgs.value<bool>(k_FixProblemVoxels_Key);
  auto pGridGeomDataPath = filterArgs.value<DataPath>(k_GridGeometryDataPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pTriangleGeometryPath = filterArgs.value<DataPath>(k_TriangleGeometryName_Key);
  auto pVertexGroupDataName = filterArgs.value<std::string>(k_VertexDataGroupName_Key);
  auto pNodeTypesName = filterArgs.value<std::string>(k_NodeTypesArrayName_Key);
  auto pFaceGroupDataName = filterArgs.value<std::string>(k_FaceDataGroupName_Key);
  auto pFaceLabelsName = filterArgs.value<std::string>(k_FaceLabelsArrayName_Key);

  DataPath pVertexGroupDataPath = pTriangleGeometryPath.createChildPath(pVertexGroupDataName);
  DataPath pFaceGroupDataPath = pTriangleGeometryPath.createChildPath(pFaceGroupDataName);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto* gridGeom = dataStructure.getDataAs<IGridGeometry>(pGridGeomDataPath);
  if(gridGeom == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-76530, fmt::format("Could not find find selected grid geometry at path '{}'", pGridGeomDataPath.toString()))};
  }
  auto numElements = gridGeom->getNumberOfCells();

  // Use FeatureIds DataStore format for created DataArrays
  const auto* featureIdsArrayPtr = dataStructure.getDataAs<IDataArray>(pFeatureIdsArrayPathValue);
  const std::string dataStoreFormat = featureIdsArrayPtr->getDataFormat();

  // Create the Triangle Geometry action and store it
  {
    auto createTriangleGeometryAction = std::make_unique<CreateTriangleGeometryAction>(pTriangleGeometryPath, numElements, 1, pVertexGroupDataName, pFaceGroupDataName,
                                                                                       CreateTriangleGeometryAction::k_DefaultVerticesName, CreateTriangleGeometryAction::k_DefaultFacesName);
    resultOutputActions.value().appendAction(std::move(createTriangleGeometryAction));
  }
  // Create the face NodesType DataArray action and store it
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::int8, std::vector<usize>{1}, std::vector<usize>{1},
                                                                 pTriangleGeometryPath.createChildPath(pVertexGroupDataName).createChildPath(pNodeTypesName), dataStoreFormat);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  // Create the face Labels DataArray action and store it
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::int32, std::vector<usize>{numElements}, std::vector<usize>{2},
                                                                 pTriangleGeometryPath.createChildPath(pFaceGroupDataName).createChildPath(pFaceLabelsName), dataStoreFormat);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  for(const auto& selectedDataPath : pSelectedDataArrayPaths)
  {
    DataPath createdDataPath = pFaceGroupDataPath.createChildPath(selectedDataPath.getTargetName());
    auto createArrayAction = std::make_unique<CopyArrayInstanceAction>(selectedDataPath, createdDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  {
    auto faceFeatureAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FaceFeatureAttributeMatrixName_Key);
    DataPath pFaceFeatureAttrMatrixPath = pTriangleGeometryPath.createChildPath(faceFeatureAttributeMatrixName);
    auto createFeatureGroupAction = std::make_unique<CreateAttributeMatrixAction>(pFaceFeatureAttrMatrixPath, std::vector<usize>{1});
    resultOutputActions.value().appendAction(std::move(createFeatureGroupAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> QuickSurfaceMeshFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel) const
{
  complex::QuickSurfaceMeshInputValues inputs;

  inputs.pGenerateTripleLines = filterArgs.value<bool>(k_GenerateTripleLines_Key);
  inputs.pFixProblemVoxels = filterArgs.value<bool>(k_FixProblemVoxels_Key);
  inputs.pGridGeomDataPath = filterArgs.value<DataPath>(k_GridGeometryDataPath_Key);
  inputs.pFeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputs.pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputs.pTriangleGeometryPath = filterArgs.value<DataPath>(k_TriangleGeometryName_Key);
  inputs.pVertexGroupDataPath = inputs.pTriangleGeometryPath.createChildPath(filterArgs.value<std::string>(k_VertexDataGroupName_Key));
  inputs.pNodeTypesDataPath = inputs.pVertexGroupDataPath.createChildPath(filterArgs.value<std::string>(k_NodeTypesArrayName_Key));
  inputs.pFaceGroupDataPath = inputs.pTriangleGeometryPath.createChildPath(filterArgs.value<std::string>(k_FaceDataGroupName_Key));
  inputs.pFaceLabelsDataPath = inputs.pFaceGroupDataPath.createChildPath(filterArgs.value<std::string>(k_FaceLabelsArrayName_Key));

  MultiArraySelectionParameter::ValueType createdDataPaths;
  for(const auto& selectedDataPath : inputs.pSelectedDataArrayPaths)
  {
    DataPath createdDataPath = inputs.pFaceGroupDataPath.createChildPath(selectedDataPath.getTargetName());
    createdDataPaths.push_back(createdDataPath);
  }
  inputs.pCreatedDataArrayPaths = createdDataPaths;

  return complex::QuickSurfaceMesh(dataStructure, &inputs, shouldCancel, messageHandler)();
}
} // namespace complex
