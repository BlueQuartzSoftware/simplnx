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
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

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
  return "Create Surface Mesh (Surface Nets)";
}

//------------------------------------------------------------------------------
std::vector<std::string> SurfaceNetsFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Generation", "Create", "Triangle", "Geometry", "SurfaceNets"};
}

//------------------------------------------------------------------------------
Parameters SurfaceNetsFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_RepairTriangleWinding_Key, "Attempt to Make Windings Consistent",
                                                "If true, attempts to repair the windings for the mesh. This may not be possible. See help page.", true));
  params.insert(std::make_unique<ChoicesParameter>(k_BoundingBoxSkinMode_Key, "Bounding Box Skin",
                                                   "Controls how triangles are generated on the outer wall of the bounding box. 'Off' generates the "
                                                   "wall as normal. 'Background-Backed Walls Only' omits wall faces where the wall borders the "
                                                   "background (Feature Id 0); faces where the wall caps a real Feature ARE still generated, so "
                                                   "Features flush with the box stay closed.",
                                                   BoundingBoxSkinMode::k_Off, ChoicesParameter::Choices{"Off", "Background-Backed Walls Only"}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplySmoothing_Key, "Apply smoothing operations", "Use the built in smoothing operation.", false));
  params.insert(std::make_unique<Int32Parameter>(k_SmoothingIterations_Key, "Relaxation Iterations", "Number of relaxation iterations to perform. More iterations causes more smoothing.", 20));
  params.insert(
      std::make_unique<Float32Parameter>(k_MaxDistanceFromVoxelCenter_Key, "Max Distance from Voxel Center", "The maximum allowable distance that a node can move from the voxel center", 1.0F));
  params.insert(std::make_unique<Float32Parameter>(k_RelaxationFactor_Key, "Relaxation Factor", "The factor used to determine how far a node can move in each smoothing iteration", 0.5F));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GridGeometryDataPath_Key, "Input Image Geometry", "DataPath to input Image Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(
      k_SelectedDataArrayPaths_Key, "Cell Attribute Arrays to Transfer", "The paths to the Arrays specifying which Cell Attribute Arrays to transfer to the created Triangle Geometry",
      MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(
      k_SelectedFeatureDataArrayPaths_Key, "Feature Attribute Arrays to Transfer", "The paths to the Arrays specifying which feature Attribute Arrays to transfer to the created Triangle Geometry",
      MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Output Triangle Geometry"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_CreatedTriangleGeometryPath_Key, "Created Triangle Geometry", "The name of the created Triangle Geometry", DataPath({"TriangleDataContainer"})));

  params.insertSeparator(Parameters::Separator{"Output Vertex Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexDataGroupName_Key, "Vertex Data [AttributeMatrix]",
                                                          "The complete path to the DataGroup where the Vertex Data of the Triangle Geometry will be created",
                                                          INodeGeometry0D::k_VertexAttributeMatrixName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NodeTypesArrayName_Key, "Node Type", "The complete path to the Array specifying the type of node in the Triangle Geometry", "NodeTypes"));

  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceDataGroupName_Key, "Face Data [AttributeMatrix]",
                                                          "The complete path to the DataGroup where the Face Data of the Triangle Geometry will be created",
                                                          INodeGeometry2D::k_FaceAttributeMatrixName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceLabelsArrayName_Key, "Face Labels",
                                                          "The complete path to the Array specifying which Features are on either side of each Face in the Triangle Geometry", "FaceLabels"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType SurfaceNetsFilter::parametersVersion() const
{
  return 2;
  // Version 1 -> 2
  // Change 1:
  // Added - k_BoundingBoxSkinMode_Key = "bounding_box_skin_mode_index";
  // Solution - set the value to 0 (BoundingBoxSkinMode::k_Off, preserves prior behavior);
  //
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SurfaceNetsFilter::clone() const
{
  return std::make_unique<SurfaceNetsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SurfaceNetsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                          const ExecutionContext& executionContext) const
{
  auto pGridGeomDataPath = filterArgs.value<DataPath>(k_GridGeometryDataPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  auto pTriangleGeometryPath = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  auto pVertexGroupDataName = filterArgs.value<std::string>(k_VertexDataGroupName_Key);
  auto pNodeTypesName = filterArgs.value<std::string>(k_NodeTypesArrayName_Key);
  auto pFaceGroupDataName = filterArgs.value<std::string>(k_FaceDataGroupName_Key);
  auto pFaceLabelsName = filterArgs.value<std::string>(k_FaceLabelsArrayName_Key);
  auto pFeatureDataPaths = filterArgs.value<std::vector<DataPath>>(k_SelectedFeatureDataArrayPaths_Key);

  DataPath pVertexGroupDataPath = pTriangleGeometryPath.createChildPath(pVertexGroupDataName);
  DataPath pFaceGroupDataPath = pTriangleGeometryPath.createChildPath(pFaceGroupDataName);

  nx::core::Result<OutputActions> resultOutputActions;

  const auto& gridGeom = dataStructure.getDataRefAs<IGridGeometry>(pGridGeomDataPath);
  constexpr usize numElements = 0;

  // Use FeatureIds DataStore format for created DataArrays
  const auto* featureIdsArrayPtr = dataStructure.getDataAs<IDataArray>(pFeatureIdsArrayPathValue);
  const std::string dataStoreFormat = featureIdsArrayPtr->getDataFormat();

  // Create the Triangle Geometry action and store it
  {
    auto createTriangleGeometryAction = std::make_unique<CreateTriangleGeometryAction>(pTriangleGeometryPath, numElements, 1, pVertexGroupDataName, pFaceGroupDataName,
                                                                                       TriangleGeom::k_SharedVertexListName, TriangleGeom::k_SharedFacesListName);
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
    for(const DataPath& selectedDataPath : pFeatureDataPaths)
    {
      // Check that the feature array has the correct tuple count to avoid crashing in execute.
      const IDataArray* featureArray = dataStructure.getDataAs<IDataArray>(selectedDataPath);
      DataPath createdDataPath = pFaceGroupDataPath.createChildPath(selectedDataPath.getTargetName());
      const auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(selectedDataPath);
      auto compShape = iDataArray.getComponentShape();
      // Double the size of the DataArray because we need the value from both sides of the triangle.
      compShape.insert(compShape.begin(), 2);

      auto createArrayAction = std::make_unique<CreateArrayAction>(iDataArray.getDataType(), std::vector<usize>{numElements}, compShape, createdDataPath, dataStoreFormat);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> SurfaceNetsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  SurfaceNetsInputValues inputValues;

  inputValues.ApplySmoothing = filterArgs.value<bool>(k_ApplySmoothing_Key);
  inputValues.RepairTriangleWinding = filterArgs.value<bool>(k_RepairTriangleWinding_Key);
  inputValues.BoundingBoxSkinMode = filterArgs.value<ChoicesParameter::ValueType>(k_BoundingBoxSkinMode_Key);
  inputValues.SmoothingIterations = filterArgs.value<int32>(k_SmoothingIterations_Key);
  inputValues.MaxDistanceFromVoxel = filterArgs.value<float32>(k_MaxDistanceFromVoxelCenter_Key);
  inputValues.RelaxationFactor = filterArgs.value<float32>(k_RelaxationFactor_Key);

  inputValues.GridGeomDataPath = filterArgs.value<DataPath>(k_GridGeometryDataPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.SelectedCellDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputValues.SelectedFeatureDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedFeatureDataArrayPaths_Key);
  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  inputValues.VertexGroupDataPath = inputValues.TriangleGeometryPath.createChildPath(filterArgs.value<std::string>(k_VertexDataGroupName_Key));
  inputValues.NodeTypesDataPath = inputValues.VertexGroupDataPath.createChildPath(filterArgs.value<std::string>(k_NodeTypesArrayName_Key));
  inputValues.FaceGroupDataPath = inputValues.TriangleGeometryPath.createChildPath(filterArgs.value<std::string>(k_FaceDataGroupName_Key));
  inputValues.FaceLabelsDataPath = inputValues.FaceGroupDataPath.createChildPath(filterArgs.value<std::string>(k_FaceLabelsArrayName_Key));

  MultiArraySelectionParameter::ValueType createdDataPaths;
  for(const auto& selectedDataPath : inputValues.SelectedCellDataArrayPaths)
  {
    createdDataPaths.push_back(inputValues.FaceGroupDataPath.createChildPath(selectedDataPath.getTargetName()));
  }
  for(const auto& selectedDataPath : inputValues.SelectedFeatureDataArrayPaths)
  {
    DataPath createdDataPath = inputValues.FaceGroupDataPath.createChildPath(selectedDataPath.getTargetName());
    createdDataPaths.push_back(createdDataPath);
  }
  inputValues.CreatedDataArrayPaths = createdDataPaths;

  return SurfaceNets(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
