#include "ExtractInternalSurfacesFromTriangleGeometryFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ExtractInternalSurfacesFromTriangleGeometry.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
constexpr int32 k_MissingTriangleVerticesArray = -351;
constexpr int32 k_MissingTriangleFacesArray = -352;
constexpr int32 k_NoNodeTypesArray = -353;
constexpr int32 k_MissingVertexArray = -354;
constexpr int32 k_MissingTriangleArray = -355;

} // namespace

namespace nx::core
{

//------------------------------------------------------------------------------
std::string ExtractInternalSurfacesFromTriangleGeometryFilter::name() const
{
  return FilterTraits<ExtractInternalSurfacesFromTriangleGeometryFilter>::name;
}

//------------------------------------------------------------------------------
std::string ExtractInternalSurfacesFromTriangleGeometryFilter::className() const
{
  return FilterTraits<ExtractInternalSurfacesFromTriangleGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ExtractInternalSurfacesFromTriangleGeometryFilter::uuid() const
{
  return FilterTraits<ExtractInternalSurfacesFromTriangleGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ExtractInternalSurfacesFromTriangleGeometryFilter::humanName() const
{
  return "Extract Internal Surfaces From Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExtractInternalSurfacesFromTriangleGeometryFilter::defaultTags() const
{
  return {className(), "Geometry", "Triangle Geometry", "Memory Management"};
}

//------------------------------------------------------------------------------
Parameters ExtractInternalSurfacesFromTriangleGeometryFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Triangle Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedTriangleGeometryPath_Key, "Triangle Geometry", "Path to the existing Triangle Geometry", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Input Vertex Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NodeTypesPath_Key, "Node Types Array", "Path to the Node Types array", DataPath(), ArraySelectionParameter::AllowedTypes{DataType::int8},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<VectorInt8Parameter>(k_NodeTypeRange_Key, "Internal Surface Node Type Min & Max",
                                                      "The min and max (inclusive) Node Type values that distinguish an internal surface from an external surface", std::vector<int8>{0, 8},
                                                      std::vector<std::string>{"Min", "Max"}));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedTriangleGeometryPath_Key, "Created Triangle Geometry Path", "Path to create the new Triangle Geometry", DataPath()));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAttributeMatrixName_Key, "Vertex Data Attribute Matrix", "Created vertex data AttributeMatrix name",
                                                          INodeGeometry0D::k_VertexAttributeMatrixName));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_TriangleAttributeMatrixName_Key, "Face Data Attribute Matrix", "Created face data AttributeMatrix name", INodeGeometry2D::k_FaceAttributeMatrixName));

  params.insertSeparator(Parameters::Separator{"Optional Transferred Data"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CopyVertexPaths_Key, "Copy Vertex Arrays", "Paths to vertex-related DataArrays that should be copied to the new geometry",
                                                               std::vector<DataPath>{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllDataTypes()));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CopyTrianglePaths_Key, "Copy Face Arrays", "Paths to face-related DataArrays that should be copied to the new geometry",
                                                               std::vector<DataPath>{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllDataTypes()));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ExtractInternalSurfacesFromTriangleGeometryFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractInternalSurfacesFromTriangleGeometryFilter::clone() const
{
  return std::make_unique<ExtractInternalSurfacesFromTriangleGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractInternalSurfacesFromTriangleGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto triangleGeomPath = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  auto internalTrianglesGeomPath = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  auto nodeTypesArrayPath = filterArgs.value<DataPath>(k_NodeTypesPath_Key);
  auto copyVertexPaths = filterArgs.value<std::vector<DataPath>>(k_CopyVertexPaths_Key);
  auto copyTrianglePaths = filterArgs.value<std::vector<DataPath>>(k_CopyTrianglePaths_Key);
  auto vertexDataName = filterArgs.value<std::string>(k_VertexAttributeMatrixName_Key);
  auto faceDataName = filterArgs.value<std::string>(k_TriangleAttributeMatrixName_Key);

  OutputActions actions;
  const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomPath);

  // Validate NodeTypes and SharedVertexList all have the same number of tuples
  {
    if(triangleGeom.getVertices() == nullptr)
    {
      std::string ss = fmt::format("Triangle Geometry does not have an assigned vertices array");
      return {MakeErrorResult<OutputActions>(k_MissingTriangleVerticesArray, ss)};
    }
    std::vector<DataPath> vertexArrays;
    vertexArrays.push_back(triangleGeom.getVertices()->getDataPaths().front());
    const auto* nodeTypesPtr = dataStructure.getDataAs<Int8Array>(nodeTypesArrayPath);
    if(nodeTypesPtr == nullptr)
    {
      std::string ss("Node Types array not found at path '{}'. Array must be of type Int8");
      return {MakeErrorResult<OutputActions>(k_NoNodeTypesArray, ss)};
    }
    vertexArrays.push_back(nodeTypesArrayPath);

    auto tupleValidityCheck = dataStructure.validateNumberOfTuples(vertexArrays);
    if(!tupleValidityCheck)
    {
      return MakePreflightErrorResult(-2071, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()));
    }
  }

  if(triangleGeom.getFaces() == nullptr)
  {
    std::string ss = fmt::format("Triangle Geometry does not a Shared Face List");
    return {MakeErrorResult<OutputActions>(k_MissingTriangleFacesArray, ss)};
  }

  ShapeType cDims(1, 1);

  // Create Geometry
  usize numFaces = triangleGeom.getNumberOfFaces();
  usize numVertices = triangleGeom.getNumberOfVertices();
  auto createInternalTrianglesAction = std::make_unique<CreateTriangleGeometryAction>(internalTrianglesGeomPath, numFaces, numVertices, vertexDataName, faceDataName,
                                                                                      TriangleGeom::k_SharedVertexListName, TriangleGeom::k_SharedFacesListName);
  DataPath internalVertexDataPath = createInternalTrianglesAction->getVertexDataPath();
  DataPath internalFaceDataPath = createInternalTrianglesAction->getFaceDataPath();
  actions.appendAction(std::move(createInternalTrianglesAction));

  ShapeType tDims(1, 0);
  std::list<std::string> tempDataArrayList;

  // Create arrays and check number of tuples match their respective face or vertex attribute matrix
  std::vector<DataPath> copiedArrays;
  copiedArrays.push_back(triangleGeom.getVertices()->getDataPaths().front());
  for(const auto& data_array : copyVertexPaths)
  {
    copiedArrays.push_back(data_array);
    auto targetDataArray = dataStructure.getDataAs<IDataArray>(data_array);
    if(targetDataArray == nullptr)
    {
      std::string ss = fmt::format("Could not find DataArray at path '{}'", data_array.toString());
      return {MakeErrorResult<OutputActions>(k_MissingVertexArray, ss)};
    }

    DataType type = targetDataArray->getDataType();
    DataPath copyPath = internalVertexDataPath.createChildPath(data_array.getTargetName());
    auto numTuples = targetDataArray->getNumberOfTuples();
    auto components = targetDataArray->getNumberOfComponents();
    const std::string dataStoreFormat = targetDataArray->getDataFormat();

    auto action = std::make_unique<CreateArrayAction>(type, std::vector<usize>{numTuples}, std::vector<usize>{components}, copyPath, dataStoreFormat);
    actions.appendAction(std::move(action));
  }
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(copiedArrays);
  if(!tupleValidityCheck)
  {
    return MakePreflightErrorResult(-2071, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()));
  }

  copiedArrays.clear();
  copiedArrays.push_back(triangleGeom.getFaces()->getDataPaths().front());
  for(const auto& data_array : copyTrianglePaths)
  {
    copiedArrays.push_back(data_array);
    auto targetDataArray = dataStructure.getDataAs<IDataArray>(data_array);
    if(targetDataArray == nullptr)
    {
      std::string ss = fmt::format("Could not find DataArray at path '{}'", data_array.toString());
      return {MakeErrorResult<OutputActions>(k_MissingTriangleArray, ss)};
    }

    DataType type = targetDataArray->getDataType();
    DataPath copyPath = internalFaceDataPath.createChildPath(data_array.getTargetName());
    auto numTuples = targetDataArray->getNumberOfTuples();
    auto components = targetDataArray->getNumberOfComponents();
    const std::string dataStoreFormat = targetDataArray->getDataFormat();

    auto action = std::make_unique<CreateArrayAction>(type, std::vector<usize>{numTuples}, std::vector<usize>{components}, copyPath, dataStoreFormat);
    actions.appendAction(std::move(action));
  }
  tupleValidityCheck = dataStructure.validateNumberOfTuples(copiedArrays);
  if(!tupleValidityCheck)
  {
    return MakePreflightErrorResult(-2071, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()));
  }

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ExtractInternalSurfacesFromTriangleGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode,
                                                                        const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ExtractInternalSurfacesFromTriangleGeometryInputValues inputValues;
  inputValues.CopyTriangleArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_CopyTrianglePaths_Key);
  inputValues.CopyVertexArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_CopyVertexPaths_Key);
  inputValues.InputTriangleGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_SelectedTriangleGeometryPath_Key);
  inputValues.NodeTypeRange = filterArgs.value<VectorInt8Parameter::ValueType>(k_NodeTypeRange_Key);
  inputValues.NodeTypesPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_NodeTypesPath_Key);
  inputValues.OutputTriangleGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_CreatedTriangleGeometryPath_Key);
  inputValues.TriangleAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_TriangleAttributeMatrixName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  return ExtractInternalSurfacesFromTriangleGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_TriangleDataContainerNameKey = "TriangleDataContainerName";
constexpr StringLiteral k_NodeTypesArrayPathKey = "NodeTypesArrayPath";
constexpr StringLiteral k_InternalTrianglesNameKey = "InternalTrianglesName";
} // namespace SIMPL
} // namespace

Result<Arguments> ExtractInternalSurfacesFromTriangleGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ExtractInternalSurfacesFromTriangleGeometryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_TriangleDataContainerNameKey, k_SelectedTriangleGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NodeTypesArrayPathKey, k_NodeTypesPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_InternalTrianglesNameKey, k_CreatedTriangleGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
