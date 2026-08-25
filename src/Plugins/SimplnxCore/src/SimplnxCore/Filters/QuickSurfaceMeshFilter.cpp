#include "QuickSurfaceMeshFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/QuickSurfaceMesh.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CopyArrayInstanceAction.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
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
  return "Create Surface Mesh (QuickMesh)";
}

//------------------------------------------------------------------------------
std::vector<std::string> QuickSurfaceMeshFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Generation", "Create", "Triangle", "Geometry"};
}

//------------------------------------------------------------------------------
Parameters QuickSurfaceMeshFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<BoolParameter>(k_FixProblemVoxels_Key, "Attempt to Fix Problem Voxels", "See help page.", false));
  params.insert(std::make_unique<BoolParameter>(k_RepairTriangleWinding_Key, "Attempt to Make Windings Consistent",
                                                "If true, attempts to repair the windings for the mesh. This may not be possible. See help page.", true));
  params.insert(std::make_unique<ChoicesParameter>(k_BoundingBoxSkinMode_Key, "Bounding Box Skin",
                                                   "Controls how triangles are generated on the outer wall of the bounding box. 'Off' generates the "
                                                   "wall as normal. 'Background-Backed Walls Only' omits wall faces where the wall borders the "
                                                   "background (Feature Id 0); faces where the wall caps a real Feature ARE still generated, so "
                                                   "Features flush with the box stay closed.",
                                                   BoundingBoxSkinMode::k_Off, ChoicesParameter::Choices{"Off", "Background-Backed Walls Only"}));

  params.insert(std::make_unique<GeometrySelectionParameter>(k_GridGeometryDataPath_Key, "Grid Geometry", "The complete path to the Grid Geometry from which to create a Triangle Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image, IGeometry::Type::RectGrid}));
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
  params.insert(std::make_unique<DataObjectNameParameter>(k_NodeTypesArrayName_Key, "Node Type", "The name of the Array specifying the type of node in the Triangle Geometry", "NodeTypes"));

  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceDataGroupName_Key, "Face Data [AttributeMatrix]",
                                                          "The complete path to the DataGroup where the Face Data of the Triangle Geometry will be created",
                                                          INodeGeometry2D::k_FaceAttributeMatrixName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FaceLabelsArrayName_Key, "Face Labels",
                                                          "The name of the Array specifying which Features are on either side of each Face in the Triangle Geometry", "FaceLabels"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType QuickSurfaceMeshFilter::parametersVersion() const
{
  return 3;
  // Version 1 -> 2
  // Change 1:
  // Added - k_RepairTriangleWinding_Key = "repair_triangle_winding";
  // Solution - set the value to false (not default);
  //
  // Version 2 -> 3
  // Change 1:
  // Added - k_BoundingBoxSkinMode_Key = "bounding_box_skin_mode_index";
  // Solution - set the value to 0 (BoundingBoxSkinMode::k_Off, preserves prior behavior);
  //
}

//------------------------------------------------------------------------------
IFilter::UniquePointer QuickSurfaceMeshFilter::clone() const
{
  return std::make_unique<QuickSurfaceMeshFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult QuickSurfaceMeshFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
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
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& gridGeom = dataStructure.getDataRefAs<IGridGeometry>(pGridGeomDataPath);

  const usize elementTupleCount = gridGeom.getCellData()->getNumberOfTuples();
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
    // Check that the feature array has the correct tuple count to avoid crashing in execute.
    const IDataArray* elementArray = dataStructure.getDataAs<IDataArray>(selectedDataPath);
    if(elementArray->getNumberOfTuples() != elementTupleCount)
    {
      return {MakeErrorResult<OutputActions>(-76531, fmt::format("Cannot copy element data at path '{}'. DataArray does not have the correct tuple count.", selectedDataPath.toString()))};
    }

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

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> QuickSurfaceMeshFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  nx::core::QuickSurfaceMeshInputValues inputValues;

  inputValues.FixProblemVoxels = filterArgs.value<bool>(k_FixProblemVoxels_Key);
  inputValues.RepairTriangleWinding = filterArgs.value<bool>(k_RepairTriangleWinding_Key);
  inputValues.BoundingBoxSkinMode = filterArgs.value<ChoicesParameter::ValueType>(k_BoundingBoxSkinMode_Key);

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
    DataPath createdDataPath = inputValues.FaceGroupDataPath.createChildPath(selectedDataPath.getTargetName());
    createdDataPaths.push_back(createdDataPath);
  }
  for(const auto& selectedDataPath : inputValues.SelectedFeatureDataArrayPaths)
  {
    DataPath createdDataPath = inputValues.FaceGroupDataPath.createChildPath(selectedDataPath.getTargetName());
    createdDataPaths.push_back(createdDataPath);
  }
  inputValues.CreatedDataArrayPaths = createdDataPaths;

  return nx::core::QuickSurfaceMesh(dataStructure, &inputValues, shouldCancel, messageHandler)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FixProblemVoxelsKey = "FixProblemVoxels";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_SelectedDataArrayPathsKey = "SelectedDataArrayPaths";
constexpr StringLiteral k_SurfaceDataContainerNameKey = "SurfaceDataContainerName";
constexpr StringLiteral k_VertexAttributeMatrixNameKey = "VertexAttributeMatrixName";
constexpr StringLiteral k_NodeTypesArrayNameKey = "NodeTypesArrayName";
constexpr StringLiteral k_FaceAttributeMatrixNameKey = "FaceAttributeMatrixName";
constexpr StringLiteral k_FaceLabelsArrayNameKey = "FaceLabelsArrayName";
constexpr StringLiteral k_FeatureAttributeMatrixNameKey = "FeatureAttributeMatrixName";
} // namespace SIMPL
} // namespace

Result<Arguments> QuickSurfaceMeshFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = QuickSurfaceMeshFilter().getDefaultArguments();

  std::vector<Result<>> results;

  Result<> fixResult = SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FixProblemVoxelsKey, k_FixProblemVoxels_Key);
  if(fixResult.valid())
  {
    // This parameter does not appear in 6.5, thus we only include it in the output if it's valid
    results.push_back(std::move(fixResult));
  }
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_GridGeometryDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedDataArrayPathsKey, k_SelectedDataArrayPaths_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DCPathBuilderFilterParameterConverter>(args, json, SIMPL::k_SurfaceDataContainerNameKey, k_CreatedTriangleGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_VertexAttributeMatrixNameKey, k_VertexDataGroupName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NodeTypesArrayNameKey, k_NodeTypesArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FaceAttributeMatrixNameKey, k_FaceDataGroupName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FaceLabelsArrayNameKey, k_FaceLabelsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
