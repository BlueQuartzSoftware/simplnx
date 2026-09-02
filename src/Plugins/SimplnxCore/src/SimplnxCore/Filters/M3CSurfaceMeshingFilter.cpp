#include "M3CSurfaceMeshingFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/M3CSurfaceMeshing.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string M3CSurfaceMeshingFilter::name() const
{
  return FilterTraits<M3CSurfaceMeshingFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string M3CSurfaceMeshingFilter::className() const
{
  return FilterTraits<M3CSurfaceMeshingFilter>::className;
}

//------------------------------------------------------------------------------
Uuid M3CSurfaceMeshingFilter::uuid() const
{
  return FilterTraits<M3CSurfaceMeshingFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string M3CSurfaceMeshingFilter::humanName() const
{
  return "Create Surface Mesh (M3C Multi-Material Marching Cubes)";
}

//------------------------------------------------------------------------------
std::vector<std::string> M3CSurfaceMeshingFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Generation", "Create", "Triangle", "Geometry", "M3C", "Marching Cubes"};
}

//------------------------------------------------------------------------------
Parameters M3CSurfaceMeshingFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_RepairTriangleWinding_Key, "Attempt to Make Windings Consistent",
                                                "If true, runs a winding-consistency repair pass after meshing. The M3C per-triangle winding heuristic does not "
                                                "guarantee globally consistent normals, so this is recommended.",
                                                true));
  params.insert(std::make_unique<ChoicesParameter>(k_BoundingBoxSkinMode_Key, "Bounding Box Skin",
                                                   "Controls how triangles are generated on the outer wall of the bounding box. 'Off' generates the "
                                                   "wall as normal. 'Background-Backed Walls Only' omits wall faces where the wall borders the "
                                                   "background (Feature Id 0); faces where the wall caps a real Feature ARE still generated, so "
                                                   "Features flush with the box stay closed.",
                                                   BoundingBoxSkinMode::k_Off, ChoicesParameter::Choices{"Off", "Background-Backed Walls Only"}));
  params.insert(std::make_unique<BoolParameter>(k_SharpBoundingBoxEdges_Key, "Sharp Bounding Box Edges",
                                                "M3C places mesh vertices on cell edges, so on its own it bevels the edges and corners of the bounding box by half a cell (a 45 "
                                                "degree chamfer). If true, the wall vertices along each box edge are moved onto the edge and merged, and the chamfer "
                                                "triangles are removed, so the six walls meet at sharp edges and corners as a finite element model expects.",
                                                true));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GridGeometryDataPath_Key, "Image Geometry", "The complete path to the Image Geometry from which to create a Triangle Geometry",
                                                             DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(
      k_SelectedDataArrayPaths_Key, "Cell Attribute Arrays to Transfer", "The Cell Attribute Arrays to transfer to the created Triangle Geometry (one value per side of each face)",
      MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(
      k_SelectedFeatureDataArrayPaths_Key, "Feature Attribute Arrays to Transfer", "The Feature Attribute Arrays to transfer to the created Triangle Geometry (one value per side of each face)",
      MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Output Triangle Geometry"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_CreatedTriangleGeometryPath_Key, "Created Triangle Geometry", "The name of the created Triangle Geometry", DataPath({"TriangleDataContainer"})));

  params.insertSeparator(Parameters::Separator{"Output Vertex Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexDataGroupName_Key, "Vertex Data [AttributeMatrix]",
                                                          "The name of the AttributeMatrix where the Vertex Data of the Triangle Geometry will be created",
                                                          INodeGeometry0D::k_VertexAttributeMatrixName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NodeTypesArrayName_Key, "Node Type", "The name of the Array specifying the type of node in the Triangle Geometry", "NodeTypes"));

  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceDataGroupName_Key, "Face Data [AttributeMatrix]",
                                                          "The name of the AttributeMatrix where the Face Data of the Triangle Geometry will be created", INodeGeometry2D::k_FaceAttributeMatrixName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceLabelsArrayName_Key, "Face Labels",
                                                          "The name of the Array specifying which Features are on either side of each Face in the Triangle Geometry", "FaceLabels"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType M3CSurfaceMeshingFilter::parametersVersion() const
{
  return 3;
  // Version 1 -> 2
  // Change 1:
  // Added - k_BoundingBoxSkinMode_Key = "bounding_box_skin_mode_index";
  // Solution - set the value to 0 (BoundingBoxSkinMode::k_Off, preserves prior behavior);
  //
  // Version 2 -> 3
  // Change 1:
  // Added - k_SharpBoundingBoxEdges_Key = "sharp_bounding_box_edges";
  // Solution - missing values take the parameter default (true). Note that this deliberately CHANGES the
  //            output of older pipelines: the bounding box edges are now sharp instead of chamfered.
  //
}

//------------------------------------------------------------------------------
IFilter::UniquePointer M3CSurfaceMeshingFilter::clone() const
{
  return std::make_unique<M3CSurfaceMeshingFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult M3CSurfaceMeshingFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pFeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pFeatureDataPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedFeatureDataArrayPaths_Key);
  auto pTriangleGeometryPath = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  auto pVertexGroupDataName = filterArgs.value<std::string>(k_VertexDataGroupName_Key);
  auto pNodeTypesName = filterArgs.value<std::string>(k_NodeTypesArrayName_Key);
  auto pFaceGroupDataName = filterArgs.value<std::string>(k_FaceDataGroupName_Key);
  auto pFaceLabelsName = filterArgs.value<std::string>(k_FaceLabelsArrayName_Key);

  const DataPath pFaceGroupDataPath = pTriangleGeometryPath.createChildPath(pFaceGroupDataName);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // The number of vertices and faces is not known until execute; create empty and resize in the algorithm.
  constexpr usize numElements = 0;

  // Use the FeatureIds DataStore format for the created DataArrays (in-core vs out-of-core parity).
  const auto* featureIdsArrayPtr = dataStructure.getDataAs<IDataArray>(pFeatureIdsArrayPath);
  const std::string dataStoreFormat = featureIdsArrayPtr->getDataFormat();

  // Create the Triangle Geometry
  {
    auto createTriangleGeometryAction = std::make_unique<CreateTriangleGeometryAction>(pTriangleGeometryPath, numElements, numElements, pVertexGroupDataName, pFaceGroupDataName,
                                                                                       TriangleGeom::k_SharedVertexListName, TriangleGeom::k_SharedFacesListName);
    resultOutputActions.value().appendAction(std::move(createTriangleGeometryAction));
  }
  // NodeTypes (int8, 1 component) on the vertex AttributeMatrix
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int8, std::vector<usize>{numElements}, std::vector<usize>{1},
                                                                 pTriangleGeometryPath.createChildPath(pVertexGroupDataName).createChildPath(pNodeTypesName), dataStoreFormat);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  // FaceLabels (int32, 2 components) on the face AttributeMatrix
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, std::vector<usize>{numElements}, std::vector<usize>{2},
                                                                 pTriangleGeometryPath.createChildPath(pFaceGroupDataName).createChildPath(pFaceLabelsName), dataStoreFormat);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // For each Cell/Feature array to transfer, create a matching face array with the component shape
  // doubled (a value for each of the two features on either side of every triangle face).
  const usize featureIdsTupleCount = dataStructure.getDataRefAs<IDataArray>(pFeatureIdsArrayPath).getNumberOfTuples();
  for(const auto& selectedDataPath : pSelectedDataArrayPaths)
  {
    const auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(selectedDataPath);
    if(iDataArray.getNumberOfTuples() != featureIdsTupleCount)
    {
      return {MakeErrorResult<OutputActions>(-90200, fmt::format("Cannot transfer Cell array '{}': its tuple count ({}) does not match the FeatureIds tuple count ({}).", selectedDataPath.toString(),
                                                                 iDataArray.getNumberOfTuples(), featureIdsTupleCount))};
    }
    auto compShape = iDataArray.getComponentShape();
    compShape.insert(compShape.begin(), 2);
    auto createArrayAction = std::make_unique<CreateArrayAction>(iDataArray.getDataType(), std::vector<usize>{numElements}, compShape,
                                                                 pFaceGroupDataPath.createChildPath(selectedDataPath.getTargetName()), dataStoreFormat);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  for(const auto& selectedDataPath : pFeatureDataPaths)
  {
    const auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(selectedDataPath);
    auto compShape = iDataArray.getComponentShape();
    compShape.insert(compShape.begin(), 2);
    auto createArrayAction = std::make_unique<CreateArrayAction>(iDataArray.getDataType(), std::vector<usize>{numElements}, compShape,
                                                                 pFaceGroupDataPath.createChildPath(selectedDataPath.getTargetName()), dataStoreFormat);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> M3CSurfaceMeshingFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  M3CSurfaceMeshingInputValues inputValues;

  inputValues.RepairTriangleWinding = filterArgs.value<bool>(k_RepairTriangleWinding_Key);
  inputValues.BoundingBoxSkinMode = filterArgs.value<ChoicesParameter::ValueType>(k_BoundingBoxSkinMode_Key);
  inputValues.SharpBoundingBoxEdges = filterArgs.value<bool>(k_SharpBoundingBoxEdges_Key);
  inputValues.GridGeomDataPath = filterArgs.value<DataPath>(k_GridGeometryDataPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.SelectedCellDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputValues.SelectedFeatureDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedFeatureDataArrayPaths_Key);
  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  inputValues.VertexGroupDataPath = inputValues.TriangleGeometryPath.createChildPath(filterArgs.value<std::string>(k_VertexDataGroupName_Key));
  inputValues.NodeTypesDataPath = inputValues.VertexGroupDataPath.createChildPath(filterArgs.value<std::string>(k_NodeTypesArrayName_Key));
  inputValues.FaceGroupDataPath = inputValues.TriangleGeometryPath.createChildPath(filterArgs.value<std::string>(k_FaceDataGroupName_Key));
  inputValues.FaceLabelsDataPath = inputValues.FaceGroupDataPath.createChildPath(filterArgs.value<std::string>(k_FaceLabelsArrayName_Key));

  // The transferred face arrays are created under the face AttributeMatrix, keyed by source name, in
  // the same order the algorithm sets up its transfer functions (all cell arrays, then all feature arrays).
  MultiArraySelectionParameter::ValueType createdDataPaths;
  for(const auto& selectedDataPath : inputValues.SelectedCellDataArrayPaths)
  {
    createdDataPaths.push_back(inputValues.FaceGroupDataPath.createChildPath(selectedDataPath.getTargetName()));
  }
  for(const auto& selectedDataPath : inputValues.SelectedFeatureDataArrayPaths)
  {
    createdDataPaths.push_back(inputValues.FaceGroupDataPath.createChildPath(selectedDataPath.getTargetName()));
  }
  inputValues.CreatedDataArrayPaths = createdDataPaths;

  return M3CSurfaceMeshing(dataStructure, &inputValues, shouldCancel, messageHandler)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_SurfaceDataContainerNameKey = "SurfaceDataContainerName";
constexpr StringLiteral k_VertexAttributeMatrixNameKey = "VertexAttributeMatrixName";
constexpr StringLiteral k_SurfaceMeshNodeTypesArrayNameKey = "SurfaceMeshNodeTypesArrayName";
constexpr StringLiteral k_FaceAttributeMatrixNameKey = "FaceAttributeMatrixName";
constexpr StringLiteral k_FaceLabelsArrayNameKey = "FaceLabelsArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> M3CSurfaceMeshingFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = M3CSurfaceMeshingFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // The legacy M3CSliceBySlice took the FeatureIds path; use its container as the input geometry.
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_GridGeometryDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DCPathBuilderFilterParameterConverter>(args, json, SIMPL::k_SurfaceDataContainerNameKey, k_CreatedTriangleGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_VertexAttributeMatrixNameKey, k_VertexDataGroupName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshNodeTypesArrayNameKey, k_NodeTypesArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FaceAttributeMatrixNameKey, k_FaceDataGroupName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FaceLabelsArrayNameKey, k_FaceLabelsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
