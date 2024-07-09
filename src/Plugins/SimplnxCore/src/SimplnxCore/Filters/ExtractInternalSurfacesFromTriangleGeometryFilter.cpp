#include "ExtractInternalSurfacesFromTriangleGeometryFilter.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <fmt/format.h>

#include <limits>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <unordered_map>

using namespace nx::core;

namespace
{
constexpr int32 k_MissingTriangleVerticesArray = -351;
constexpr int32 k_MissingTriangleFacesArray = -352;
constexpr int32 k_NoNodeTypesArray = -353;
constexpr int32 k_MissingVertexArray = -354;
constexpr int32 k_MissingTriangleArray = -355;

template <class T>
void hashCombine(usize& seed, const T& obj)
{
  std::hash<T> hasher;
  seed ^= hasher(obj) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct CopyDataFunctor
{
  template <typename T>
  void operator()(IDataArray& inDataPtr, IDataArray& outDataPtr, std::unordered_map<int64, int64>& elementMap) const
  {
    auto& inputDataPtr = dynamic_cast<DataArray<T>&>(inDataPtr);
    AbstractDataStore<T>& inputData = inputDataPtr.getDataStoreRef();
    auto croppedDataPtr = dynamic_cast<DataArray<T>&>(outDataPtr);
    AbstractDataStore<T>& outputData = croppedDataPtr.getDataStoreRef();

    usize nTuples = outDataPtr.getNumberOfTuples();
    usize nComps = inDataPtr.getNumberOfComponents();
    usize tmpIndex = 0;
    usize ptrIndex = 0;

    for(usize i = 0; i < nTuples; i++)
    {
      for(usize d = 0; d < nComps; d++)
      {
        tmpIndex = nComps * i + d;
        ptrIndex = nComps * elementMap[i] + d;
        outputData[tmpIndex] = inputData[ptrIndex];
      }
    }
  }
};

struct RemoveFlaggedVerticesFunctor
{
  // copy data to masked geometry
  template <class T>
  void operator()(IDataArray& inputDataPtr, IDataArray& outputDataArray, const std::vector<IGeometry::MeshIndexType>& indexMapping) const
  {
    auto& inputData = dynamic_cast<DataArray<T>&>(inputDataPtr);
    auto& outputData = dynamic_cast<DataArray<T>&>(outputDataArray);
    usize nComps = inputData.getNumberOfComponents();
    IGeometry::MeshIndexType notSeen = std::numeric_limits<IGeometry::MeshIndexType>::max();

    for(usize i = 0; i < indexMapping.size(); i++)
    {
      IGeometry::MeshIndexType newIndex = indexMapping[i];
      if(newIndex != notSeen)
      {
        for(usize compIdx = 0; compIdx < nComps; compIdx++)
        {
          usize destinationIndex = newIndex * nComps + compIdx;
          usize sourceIndex = i * nComps + compIdx;
          outputData[destinationIndex] = inputData[sourceIndex];
        }
      }
    }
  }
};

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
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_VertexAttributeMatrixName_Key, "Vertex Data Attribute Matrix", "Created vertex data AttributeMatrix name", INodeGeometry0D::k_VertexDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_TriangleAttributeMatrixName_Key, "Face Data Attribute Matrix", "Created face data AttributeMatrix name", INodeGeometry2D::k_FaceDataName));

  params.insertSeparator(Parameters::Separator{"Optional Transferred Data"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CopyVertexPaths_Key, "Copy Vertex Arrays", "Paths to vertex-related DataArrays that should be copied to the new geometry",
                                                               std::vector<DataPath>{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllDataTypes()));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CopyTrianglePaths_Key, "Copy Face Arrays", "Paths to face-related DataArrays that should be copied to the new geometry",
                                                               std::vector<DataPath>{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllDataTypes()));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractInternalSurfacesFromTriangleGeometryFilter::clone() const
{
  return std::make_unique<ExtractInternalSurfacesFromTriangleGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractInternalSurfacesFromTriangleGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                          const std::atomic_bool& shouldCancel) const
{
  auto triangleGeomPath = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  auto internalTrianglesGeomPath = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  auto nodeTypesArrayPath = filterArgs.value<DataPath>(k_NodeTypesPath_Key);
  auto copyVertexPaths = filterArgs.value<std::vector<DataPath>>(k_CopyVertexPaths_Key);
  auto copyTrianglePaths = filterArgs.value<std::vector<DataPath>>(k_CopyTrianglePaths_Key);
  auto vertexDataName = filterArgs.value<std::string>(k_VertexAttributeMatrixName_Key);
  auto faceDataName = filterArgs.value<std::string>(k_TriangleAttributeMatrixName_Key);

  std::vector<DataPath> arrays;
  OutputActions actions;

  const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomPath);

  if(triangleGeom.getVertices() == nullptr)
  {
    std::string ss = fmt::format("Triangle Geometry does not have an assigned vertices array");
    return {MakeErrorResult<OutputActions>(k_MissingTriangleVerticesArray, ss)};
  }
  arrays.push_back(triangleGeom.getVertices()->getDataPaths().front());

  if(triangleGeom.getFaces() == nullptr)
  {
    std::string ss = fmt::format("Triangle Geometry does not have an assigned faces array");
    return {MakeErrorResult<OutputActions>(k_MissingTriangleFacesArray, ss)};
  }
  arrays.push_back(triangleGeom.getFaces()->getDataPaths().front());

  std::vector<usize> cDims(1, 1);

  const auto* nodeTypesPtr = dataStructure.getDataAs<Int8Array>(nodeTypesArrayPath);
  if(nodeTypesPtr == nullptr)
  {
    std::string ss("Node Types array not found at path '{}'. Array must be of type Int8");
    return {MakeErrorResult<OutputActions>(k_NoNodeTypesArray, ss)};
  }
  arrays.push_back(nodeTypesArrayPath);

  dataStructure.validateNumberOfTuples(arrays);

  // Create Geometry
  usize numFaces = triangleGeom.getNumberOfFaces();
  usize numVertices = triangleGeom.getNumberOfVertices();
  auto createInternalTrianglesAction = std::make_unique<CreateTriangleGeometryAction>(internalTrianglesGeomPath, numFaces, numVertices, vertexDataName, faceDataName,
                                                                                      CreateTriangleGeometryAction::k_DefaultVerticesName, CreateTriangleGeometryAction::k_DefaultFacesName);
  DataPath internalVertexDataPath = createInternalTrianglesAction->getVertexDataPath();
  DataPath internalFaceDataPath = createInternalTrianglesAction->getFaceDataPath();
  actions.appendAction(std::move(createInternalTrianglesAction));

  std::vector<usize> tDims(1, 0);
  std::list<std::string> tempDataArrayList;

  // Create arrays
  for(const auto& data_array : copyVertexPaths)
  {
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
  for(const auto& data_array : copyTrianglePaths)
  {
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

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ExtractInternalSurfacesFromTriangleGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel) const
{
  auto nodeTypesArrayPath = args.value<DataPath>(k_NodeTypesPath_Key);
  auto triangleGeomPath = args.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  auto internalTrianglesPath = args.value<DataPath>(k_CreatedTriangleGeometryPath_Key);
  auto copyVertexPaths = args.value<std::vector<DataPath>>(k_CopyVertexPaths_Key);
  auto copyTrianglePaths = args.value<std::vector<DataPath>>(k_CopyTrianglePaths_Key);
  auto vertexDataName = args.value<std::string>(k_VertexAttributeMatrixName_Key);
  auto faceDataName = args.value<std::string>(k_TriangleAttributeMatrixName_Key);

  auto minMaxNodeValues = args.value<VectorInt8Parameter::ValueType>(k_NodeTypeRange_Key);

  auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomPath);
  auto& internalTriangleGeom = dataStructure.getDataRefAs<TriangleGeom>(internalTrianglesPath);
  auto& vertices = *triangleGeom.getVertices();
  auto& triangles = *triangleGeom.getFaces();
  auto numVerts = triangleGeom.getNumberOfVertices();
  auto numTris = triangleGeom.getNumberOfFaces();

  auto& nodeTypes = dataStructure.getDataRefAs<Int8Array>(nodeTypesArrayPath);

  auto internalVerticesPath = internalTrianglesPath.createChildPath(CreateTriangleGeometryAction::k_DefaultVerticesName);
  internalTriangleGeom.setVertices(*dataStructure.getDataAs<Float32Array>(internalVerticesPath));

  auto internalFacesPath = internalTrianglesPath.createChildPath(CreateTriangleGeometryAction::k_DefaultFacesName);
  internalTriangleGeom.setFaceList(*dataStructure.getDataAs<UInt64Array>(internalFacesPath));

  // int64 progIncrement = numTris / 100;
  // int64 prog = 1;
  // int64 progressInt = 0;
  // int64 counter = 0;
  using MeshIndexType = IGeometry::MeshIndexType;

  const MeshIndexType notSeen = std::numeric_limits<MeshIndexType>::max();

  std::vector<MeshIndexType> vertNewIndex(numVerts, notSeen);
  std::vector<MeshIndexType> triNewIndex(numTris, notSeen);
  MeshIndexType currentNewTriIndex = 0;
  MeshIndexType currentNewVertIndex = 0;

  // Loop over all of the triangles mapping the triangle and the vertices to the new array locations
  for(MeshIndexType triIndex = 0; triIndex < numTris; triIndex++)
  {
    MeshIndexType v0Index = triangles[3 * triIndex + 0];
    MeshIndexType v1Index = triangles[3 * triIndex + 1];
    MeshIndexType v2Index = triangles[3 * triIndex + 2];
    // Check if the NodeType is either 2, 3, 4
    if((nodeTypes[v0Index] >= minMaxNodeValues[0] && nodeTypes[v0Index] <= minMaxNodeValues[1]) && (nodeTypes[v1Index] >= minMaxNodeValues[0] && nodeTypes[v1Index] <= minMaxNodeValues[1]) &&
       (nodeTypes[v2Index] >= minMaxNodeValues[0] && nodeTypes[v2Index] <= minMaxNodeValues[1]))
    {
      // All Nodes are the correct type
      triNewIndex[triIndex] = currentNewTriIndex;
      currentNewTriIndex++; // increment the index into which this triangle would be place in the new triangle array
      // Now figure out if we have seen each vertex
      if(vertNewIndex[v0Index] == notSeen)
      {
        vertNewIndex[v0Index] = currentNewVertIndex;
        currentNewVertIndex++;
      }
      if(vertNewIndex[v1Index] == notSeen)
      {
        vertNewIndex[v1Index] = currentNewVertIndex;
        currentNewVertIndex++;
      }
      if(vertNewIndex[v2Index] == notSeen)
      {
        vertNewIndex[v2Index] = currentNewVertIndex;
        currentNewVertIndex++;
      }
    }

    if(shouldCancel)
    {
      return {};
    }
  }

  // Resize the vertex and triangle arrays
  internalTriangleGeom.resizeVertexList(currentNewVertIndex);
  internalTriangleGeom.resizeFaceList(currentNewTriIndex);
  internalTriangleGeom.getVertexAttributeMatrix()->resizeTuples({currentNewVertIndex});
  internalTriangleGeom.getFaceAttributeMatrix()->resizeTuples({currentNewTriIndex});

  IGeometry::SharedVertexList* internalVerts = internalTriangleGeom.getVertices();
  IGeometry::SharedFaceList* internalTriangles = internalTriangleGeom.getFaces();

  // Transfer the data from the old SharedVertexList to the new VertexList
  for(MeshIndexType vertIndex = 0; vertIndex < numVerts; vertIndex++)
  {
    MeshIndexType mappedIndex = vertNewIndex[vertIndex];
    if(mappedIndex != notSeen)
    {
      // Get the actual XYZ coordinate
      float x = vertices[vertIndex * 3 + 0];
      float y = vertices[vertIndex * 3 + 1];
      float z = vertices[vertIndex * 3 + 2];

      (*internalVerts)[mappedIndex * 3 + 0] = x;
      (*internalVerts)[mappedIndex * 3 + 1] = y;
      (*internalVerts)[mappedIndex * 3 + 2] = z;
    }
  }

  // Transfer the data from the old SharedTriangleList to the new TriangleList
  for(MeshIndexType triIndex = 0; triIndex < numTris; triIndex++)
  {
    MeshIndexType mappedIndex = triNewIndex[triIndex];
    if(mappedIndex != notSeen)
    {
      // Get the 3 original vertex indices for this triangle
      MeshIndexType v0 = triangles[triIndex * 3 + 0];
      MeshIndexType v1 = triangles[triIndex * 3 + 1];
      MeshIndexType v2 = triangles[triIndex * 3 + 2];

      MeshIndexType v0New = vertNewIndex[v0];
      MeshIndexType v1New = vertNewIndex[v1];
      MeshIndexType v2New = vertNewIndex[v2];

      (*internalTriangles)[mappedIndex * 3 + 0] = v0New;
      (*internalTriangles)[mappedIndex * 3 + 1] = v1New;
      (*internalTriangles)[mappedIndex * 3 + 2] = v2New;
    }
  }

  // Copy any Vertex and Triangle DataArrays to extracted surface mesh
  for(const auto& targetArrayPath : copyVertexPaths)
  {
    DataPath destinationPath = internalTrianglesPath.createChildPath(vertexDataName).createChildPath(targetArrayPath.getTargetName());
    auto& src = dataStructure.getDataRefAs<IDataArray>(targetArrayPath);
    auto& dest = dataStructure.getDataRefAs<IDataArray>(destinationPath);

    ExecuteDataFunction(RemoveFlaggedVerticesFunctor{}, src.getDataType(), src, dest, vertNewIndex);
  }

  for(const auto& targetArrayPath : copyTrianglePaths)
  {
    DataPath destinationPath = internalTrianglesPath.createChildPath(faceDataName).createChildPath(targetArrayPath.getTargetName());
    auto& src = dataStructure.getDataRefAs<IDataArray>(targetArrayPath);
    auto& dest = dataStructure.getDataRefAs<IDataArray>(destinationPath);
    dest.getIDataStore()->resizeTuples({currentNewTriIndex});

    ExecuteDataFunction(RemoveFlaggedVerticesFunctor{}, src.getDataType(), src, dest, triNewIndex);
  }

  return {};
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

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
