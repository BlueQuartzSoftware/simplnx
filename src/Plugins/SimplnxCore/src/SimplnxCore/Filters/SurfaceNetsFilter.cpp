#include "SurfaceNetsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/SurfaceNets.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string SurfaceNetsFilter::name() const
{
  return FilterTraits<SurfaceNetsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string SurfaceNetsFilter::className() const
{
  return FilterTraits<SurfaceNetsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid SurfaceNetsFilter::uuid() const
{
  return FilterTraits<SurfaceNetsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string SurfaceNetsFilter::humanName() const
{
  return "SurfaceNets Meshing Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> SurfaceNetsFilter::defaultTags() const
{
  return {"Surface Meshing", "Generation"};
}

//------------------------------------------------------------------------------
Parameters SurfaceNetsFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplySmoothing_Key, "Apply smoothing operations", "Use the built in smmothing operation.", false));

  params.insert(std::make_unique<Int32Parameter>(k_SmoothingIterations_Key, "Relaxation Iterations", "Number of relaxation iterations to perform. More iterations causes more smoothing.", 20));
  params.insert(
      std::make_unique<Float32Parameter>(k_MaxDistanceFromVoxelCenter_Key, "Max Distance from Voxel Center", "The maximum allowable distance that a node can move from the voxel center", 1.0F));
  params.insert(std::make_unique<Float32Parameter>(k_RelaxationFactor_Key, "Relaxation Factor", "The factor used to determine how far a node can move in each smoothing iteration", 0.5F));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GridGeometryDataPath_Key, "Input Image Geometry", "DataPath to input Image Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "The complete path to the Array specifying which Feature each Cell belongs to", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(
      k_SelectedDataArrayPaths_Key, "Attribute Arrays to Transfer", "The paths to the Arrays specifying which Cell Attribute Arrays to transfer to the created Triangle Geometry",
      MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Output Triangle Geometry"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_CreatedTriangleGeometryPath_Key, "Created Triangle Geometry", "The name of the created Triangle Geometry", DataPath({"TriangleDataContainer"})));

  params.insertSeparator(Parameters::Separator{"Output Vertex Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexDataGroupName_Key, "Vertex Data [AttributeMatrix]",
                                                          "The complete path to the DataGroup where the Vertex Data of the Triangle Geometry will be created", INodeGeometry0D::k_VertexDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NodeTypesArrayName_Key, "Node Type", "The complete path to the Array specifying the type of node in the Triangle Geometry", "NodeTypes"));

  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceDataGroupName_Key, "Face Data [AttributeMatrix]",
                                                          "The complete path to the DataGroup where the Face Data of the Triangle Geometry will be created", INodeGeometry2D::k_FaceDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceLabelsArrayName_Key, "Face Labels",
                                                          "The complete path to the Array specifying which Features are on either side of each Face in the Triangle Geometry", "FaceLabels"));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceFeatureAttributeMatrixName_Key, "Face Feature Data [AttributeMatrix]",
                                                          "The complete path to the DataGroup where the Feature Data will be stored.", INodeGeometry1D::k_FaceFeatureAttributeMatrix));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ApplySmoothing_Key, k_SmoothingIterations_Key, true);
  params.linkParameters(k_ApplySmoothing_Key, k_MaxDistanceFromVoxelCenter_Key, true);
  params.linkParameters(k_ApplySmoothing_Key, k_RelaxationFactor_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType SurfaceNetsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SurfaceNetsFilter::clone() const
{
  return std::make_unique<SurfaceNetsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SurfaceNetsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto pGridGeomDataPath = filterArgs.value<DataPath>(k_GridGeometryDataPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  auto pTriangleGeometryPath = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  auto pVertexGroupDataName = filterArgs.value<std::string>(k_VertexDataGroupName_Key);
  auto pNodeTypesName = filterArgs.value<std::string>(k_NodeTypesArrayName_Key);
  auto pFaceGroupDataName = filterArgs.value<std::string>(k_FaceDataGroupName_Key);
  auto pFaceLabelsName = filterArgs.value<std::string>(k_FaceLabelsArrayName_Key);

  DataPath pVertexGroupDataPath = pTriangleGeometryPath.createChildPath(pVertexGroupDataName);
  DataPath pFaceGroupDataPath = pTriangleGeometryPath.createChildPath(pFaceGroupDataName);

  nx::core::Result<OutputActions> resultOutputActions;

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
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int8, std::vector<usize>{1}, std::vector<usize>{1},
                                                                 pTriangleGeometryPath.createChildPath(pVertexGroupDataName).createChildPath(pNodeTypesName), dataStoreFormat);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  // Create the face Labels DataArray action and store it
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, std::vector<usize>{numElements}, std::vector<usize>{2},
                                                                 pTriangleGeometryPath.createChildPath(pFaceGroupDataName).createChildPath(pFaceLabelsName), dataStoreFormat);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  for(const auto& selectedDataPath : pSelectedDataArrayPaths)
  {
    DataPath createdDataPath = pFaceGroupDataPath.createChildPath(selectedDataPath.getTargetName());
    const auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(selectedDataPath);
    auto compShape = iDataArray.getComponentShape();
    // Double the size of the DataArray because we need the value from both sides of the triangle.
    compShape.insert(compShape.begin(), 2);

    auto createArrayAction = std::make_unique<CreateArrayAction>(iDataArray.getDataType(), std::vector<usize>{numElements}, compShape, createdDataPath, dataStoreFormat);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  {
    auto faceFeatureAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FaceFeatureAttributeMatrixName_Key);
    DataPath pFaceFeatureAttrMatrixPath = pTriangleGeometryPath.createChildPath(faceFeatureAttributeMatrixName);
    auto createFeatureGroupAction = std::make_unique<CreateAttributeMatrixAction>(pFaceFeatureAttrMatrixPath, std::vector<usize>{1});
    resultOutputActions.value().appendAction(std::move(createFeatureGroupAction));
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> SurfaceNetsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  SurfaceNetsInputValues inputValues;

  inputValues.ApplySmoothing = filterArgs.value<bool>(k_ApplySmoothing_Key);
  inputValues.SmoothingIterations = filterArgs.value<int32>(k_SmoothingIterations_Key);
  inputValues.MaxDistanceFromVoxel = filterArgs.value<float32>(k_MaxDistanceFromVoxelCenter_Key);
  inputValues.RelaxationFactor = filterArgs.value<float32>(k_RelaxationFactor_Key);

  inputValues.GridGeomDataPath = filterArgs.value<DataPath>(k_GridGeometryDataPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  inputValues.VertexGroupDataPath = inputValues.TriangleGeometryPath.createChildPath(filterArgs.value<std::string>(k_VertexDataGroupName_Key));
  inputValues.NodeTypesDataPath = inputValues.VertexGroupDataPath.createChildPath(filterArgs.value<std::string>(k_NodeTypesArrayName_Key));
  inputValues.FaceGroupDataPath = inputValues.TriangleGeometryPath.createChildPath(filterArgs.value<std::string>(k_FaceDataGroupName_Key));
  inputValues.FaceLabelsDataPath = inputValues.FaceGroupDataPath.createChildPath(filterArgs.value<std::string>(k_FaceLabelsArrayName_Key));

  MultiArraySelectionParameter::ValueType createdDataPaths;
  for(const auto& selectedDataPath : inputValues.SelectedDataArrayPaths)
  {
    createdDataPaths.push_back(inputValues.FaceGroupDataPath.createChildPath(selectedDataPath.getTargetName()));
  }
  inputValues.CreatedDataArrayPaths = createdDataPaths;

  return SurfaceNets(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
