#include "ExtractInternalSurfacesFromTriangleGeometry.hpp"

#include <string>

#include "fmt/format.h"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateTriangleGeomAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

using namespace complex;

namespace
{
constexpr complex::int32 k_EMPTY_PARAMETER = -350;
constexpr complex::int32 k_MissingTriangleGeometry = -351;
constexpr complex::int32 k_NoNodeTypesArray = -352;
constexpr complex::int32 k_MissingVertexArray = -353;
constexpr complex::int32 k_MissingTriangleArray = -354;

template <class T>
inline void hashCombine(usize& seed, const T& obj)
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
        outputData.setValue(tmpIndex, inputData.getValue(ptrIndex));
      }
    }
  }
};
} // namespace

namespace complex
{

std::string ExtractInternalSurfacesFromTriangleGeometry::name() const
{
  return FilterTraits<ExtractInternalSurfacesFromTriangleGeometry>::name;
}

std::string ExtractInternalSurfacesFromTriangleGeometry::className() const
{
  return FilterTraits<ExtractInternalSurfacesFromTriangleGeometry>::className;
}

Uuid ExtractInternalSurfacesFromTriangleGeometry::uuid() const
{
  return FilterTraits<ExtractInternalSurfacesFromTriangleGeometry>::uuid;
}

std::string ExtractInternalSurfacesFromTriangleGeometry::humanName() const
{
  return "Extract Internal Surfaces From Triangle Geometry";
}

Parameters ExtractInternalSurfacesFromTriangleGeometry::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeom_Key, "Triangle Geometry", "Path to the existing Triangle Geometry", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{DataObject::Type::TriangleGeom}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_InternalTriangleGeom_Key, "Interior Triangle Geometry", "Path to create the new Triangle Geometry", DataPath()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NodeTypesPath_Key, "Node Types Array", "Path to the Node Types array", DataPath()));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CopyVertexPaths_Key, "Copy Vertex Arrays", "Paths to vertex-related DataArrays that should be copied to the new geometry",
                                                               std::vector<DataPath>{}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CopyTrianglePaths_Key, "Copy Triangle Arrays", "Paths to face-related DataArrays that should be copied to the new geometry",
                                                               std::vector<DataPath>{}));
  return params;
}

IFilter::UniquePointer ExtractInternalSurfacesFromTriangleGeometry::clone() const
{
  return std::make_unique<ExtractInternalSurfacesFromTriangleGeometry>();
}

IFilter::PreflightResult ExtractInternalSurfacesFromTriangleGeometry::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto triangleGeomPath = filterArgs.value<DataPath>(k_TriangleGeom_Key);
  auto internalTrianglesGeomPath = filterArgs.value<DataPath>(k_InternalTriangleGeom_Key);
  auto nodeTypesArrayPath = filterArgs.value<DataPath>(k_NodeTypesPath_Key);
  auto copyVertexPaths = filterArgs.value<std::vector<DataPath>>(k_CopyVertexPaths_Key);
  auto copyTrianglePaths = filterArgs.value<std::vector<DataPath>>(k_CopyTrianglePaths_Key);

  std::vector<DataPath> arrays;
  OutputActions actions;

  auto* triangleGeom = dataStructure.getDataAs<TriangleGeom>(triangleGeomPath);

  if(triangleGeom == nullptr)
  {
    std::string ss = fmt::format("Triangle Geometry not found at path '{}'", triangleGeomPath.toString());
    return {MakeErrorResult<OutputActions>(k_MissingTriangleGeometry, ss)};
  }
  if(triangleGeom->getVertices() == nullptr)
  {
    std::string ss = fmt::format("Triangle Geometry does not have an assigned vertices array");
    return {MakeErrorResult<OutputActions>(k_MissingTriangleGeometry, ss)};
  }

  arrays.push_back(triangleGeom->getVertices()->getDataPaths().front());

  std::vector<usize> cDims(1, 1);

  auto* nodeTypesPtr = dataStructure.getDataAs<Int8Array>(nodeTypesArrayPath);
  if(!nodeTypesPtr)
  {
    std::string ss("Node Types array not found at path '{}'. Array must be of type Int8");
    return {MakeErrorResult<OutputActions>(k_NoNodeTypesArray, ss)};
  }
  arrays.push_back(nodeTypesArrayPath);

  dataStructure.validateNumberOfTuples(arrays);

  // Create Geometry
  IDataStore::ShapeType geomShape = {1};
  auto createInternalTrianglesAction = std::make_unique<CreateTriangleGeomAction>(internalTrianglesGeomPath, geomShape, CreateTriangleGeomAction::AdditionalData::VerticesTriangles);
  actions.actions.push_back(std::move(createInternalTrianglesAction));

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

    NumericType type = static_cast<NumericType>(targetDataArray->getDataType());
    DataPath copyPath = internalTrianglesGeomPath.createChildPath(data_array.getTargetName());
    auto numTuples = targetDataArray->getNumberOfTuples();
    auto components = targetDataArray->getNumberOfComponents();

    auto action = std::make_unique<CreateArrayAction>(type, std::vector<usize>{numTuples}, std::vector<usize>{components}, copyPath);
    actions.actions.push_back(std::move(action));
  }
  for(const auto& data_array : copyTrianglePaths)
  {
    auto targetDataArray = dataStructure.getDataAs<IDataArray>(data_array);
    if(targetDataArray == nullptr)
    {
      std::string ss = fmt::format("Could not find DataArray at path '{}'", data_array.toString());
      return {MakeErrorResult<OutputActions>(k_MissingTriangleArray, ss)};
    }

    NumericType type = static_cast<NumericType>(targetDataArray->getDataType());
    DataPath copyPath = internalTrianglesGeomPath.createChildPath(data_array.getTargetName());
    auto numTuples = targetDataArray->getNumberOfTuples();
    auto components = targetDataArray->getNumberOfComponents();

    auto action = std::make_unique<CreateArrayAction>(type, std::vector<usize>{numTuples}, std::vector<usize>{components}, copyPath);
    actions.actions.push_back(std::move(action));
  }

  return {std::move(actions)};
}

Result<> ExtractInternalSurfacesFromTriangleGeometry::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto nodeTypesArrayPath = args.value<DataPath>(k_NodeTypesPath_Key);
  auto triangleGeomPath = args.value<DataPath>(k_TriangleGeom_Key);
  auto internalTrianglesPath = args.value<DataPath>(k_InternalTriangleGeom_Key);
  auto copyVertexPaths = args.value<std::vector<DataPath>>(k_CopyVertexPaths_Key);
  auto copyTrianglePaths = args.value<std::vector<DataPath>>(k_CopyTrianglePaths_Key);

  auto* triangleGeom = data.getDataAs<TriangleGeom>(triangleGeomPath);
  auto* internalTriangleGeom = data.getDataAs<TriangleGeom>(internalTrianglesPath);
  auto& vertices = *triangleGeom->getVertices();
  auto& triangles = *triangleGeom->getFaces();
  auto numVerts = triangleGeom->getNumberOfVertices();
  auto numTris = triangleGeom->getNumberOfFaces();

  auto& nodeTypes = data.getDataRefAs<Int8Array>(nodeTypesArrayPath);

  typedef std::array<float32, 3> Vertex;

  struct ArrayHasher
  {
    usize operator()(const Vertex& vert) const
    {
      usize hash = std::hash<float32>()(vert[0]);
      hashCombine(hash, vert[1]);
      hashCombine(hash, vert[2]);
      return hash;
    }
  };

  typedef std::array<int64, 3> Triangle;
  typedef std::unordered_map<Vertex, int64, ArrayHasher> VertexMap;
  VertexMap vertexMap;
  std::unordered_map<int64, int64> internalVertexMap;
  std::unordered_map<int64, int64> internalTriMap;
  std::vector<Vertex> tmpVerts;
  tmpVerts.reserve(numVerts);
  std::vector<Triangle> tmpTris;
  tmpTris.reserve(numTris);
  std::vector<int8> tmpNodeTypes;
  tmpNodeTypes.reserve(numVerts);
  int64 vertCounter = 0;
  int64 triCounter = 0;
  int64 tmpVert0 = 0;
  int64 tmpVert1 = 0;
  int64 tmpVert2 = 0;

  int64 progIncrement = numTris / 100;
  int64 prog = 1;
  int64 progressInt = 0;
  int64 counter = 0;

  for(usize i = 0; i < numTris; i++)
  {
    // if(getCancel())
    //{
    //  return;
    //}
    if((nodeTypes[triangles[3 * i + 0]] == 2 || nodeTypes[triangles[3 * i + 0]] == 3 || nodeTypes[triangles[3 * i + 0]] == 4) &&
       (nodeTypes[triangles[3 * i + 1]] == 2 || nodeTypes[triangles[3 * i + 1]] == 3 || nodeTypes[triangles[3 * i + 1]] == 4) &&
       (nodeTypes[triangles[3 * i + 2]] == 2 || nodeTypes[triangles[3 * i + 2]] == 3 || nodeTypes[triangles[3 * i + 2]] == 4))
    {
      Vertex tmpCoords1 = {{vertices[3 * triangles[3 * i + 0] + 0], vertices[3 * triangles[3 * i + 0] + 1], vertices[3 * triangles[3 * i + 0] + 2]}};
      Vertex tmpCoords2 = {{vertices[3 * triangles[3 * i + 1] + 0], vertices[3 * triangles[3 * i + 1] + 1], vertices[3 * triangles[3 * i + 1] + 2]}};
      Vertex tmpCoords3 = {{vertices[3 * triangles[3 * i + 2] + 0], vertices[3 * triangles[3 * i + 2] + 1], vertices[3 * triangles[3 * i + 2] + 2]}};

      auto iter = vertexMap.find(tmpCoords1);
      if(iter == vertexMap.end())
      {
        tmpVerts.push_back(tmpCoords1);
        tmpNodeTypes.push_back(nodeTypes[triangles[3 * i + 0]]);
        vertexMap[tmpCoords1] = vertCounter;
        tmpVert0 = vertCounter;
        internalVertexMap[tmpVert0] = triangles[3 * i + 0];
        vertCounter++;
      }
      else
      {
        tmpVert0 = vertexMap[tmpCoords1];
      }

      iter = vertexMap.find(tmpCoords2);
      if(iter == vertexMap.end())
      {
        tmpVerts.push_back(tmpCoords2);
        tmpNodeTypes.push_back(nodeTypes[triangles[3 * i + 1]]);
        vertexMap[tmpCoords2] = vertCounter;
        tmpVert1 = vertCounter;
        internalVertexMap[tmpVert1] = triangles[3 * i + 1];
        vertCounter++;
      }
      else
      {
        tmpVert1 = vertexMap[tmpCoords2];
      }

      iter = vertexMap.find(tmpCoords3);
      if(iter == vertexMap.end())
      {
        tmpVerts.push_back(tmpCoords3);
        tmpNodeTypes.push_back(nodeTypes[triangles[3 * i + 2]]);
        vertexMap[tmpCoords3] = vertCounter;
        tmpVert2 = vertCounter;
        internalVertexMap[tmpVert2] = triangles[3 * i + 2];
        vertCounter++;
      }
      else
      {
        tmpVert2 = vertexMap[tmpCoords3];
      }

      Triangle tmpTri = {{tmpVert0, tmpVert1, tmpVert2}};
      tmpTris.push_back(tmpTri);
      internalTriMap[triCounter] = i;
      triCounter++;
    }

    if(counter > prog)
    {
      progressInt = static_cast<int64>((static_cast<float>(counter) / numTris) * 100.0f);
      // std::string ss = fmt::format("Checking Triangle {} of {} || {}% Completed", counter, numTris, progressInt);
      // notifyStatusMessage(ss);
      prog = prog + progIncrement;
    }
    counter++;
  }

  // std::string ss = fmt::format("Finished Checking Triangles || Updating Array Information...");
  // notifyStatusMessage(ss);

  tmpVerts.shrink_to_fit();
  tmpTris.shrink_to_fit();

  std::vector<usize> vertDims(1, tmpVerts.size());
  std::vector<usize> triDims(1, tmpTris.size());

  for(const auto& copyPath : copyVertexPaths)
  {
    auto destinationPath = internalTrianglesPath.createChildPath(copyPath.getTargetName());

    auto& src = data.getDataRefAs<IDataArray>(copyPath);
    auto& dest = data.getDataRefAs<IDataArray>(destinationPath);

    assert(src.getNumberOfComponents() == dest.getNumberOfComponents());

    ExecuteDataFunction(CopyDataFunctor{}, src.getDataType(), src, dest, internalVertexMap);
  }
  for(const auto& trianglePath : copyTrianglePaths)
  {
    auto destinationPath = internalTrianglesPath.createChildPath(trianglePath.getTargetName());

    auto& src = data.getDataRefAs<IDataArray>(trianglePath);
    auto& dest = data.getDataRefAs<IDataArray>(destinationPath);

    assert(src.getNumberOfComponents() == dest.getNumberOfComponents());

    ExecuteDataFunction(CopyDataFunctor{}, src.getDataType(), src, dest, internalTriMap);
  }

  auto& internalTris = data.getDataRefAs<TriangleGeom>(internalTrianglesPath);
  internalTris.resizeVertexList(tmpVerts.size());
  internalTris.resizeFaceList(tmpTris.size());
  auto& internalVertices = *internalTris.getVertices();
  auto& internalTriangles = *internalTris.getFaces();
  auto numInternalTris = internalTris.getNumberOfFaces();
  auto numInternalVerts = internalTris.getNumberOfVertices();

  for(usize i = 0; i < numInternalVerts; i++)
  {
    internalVertices[3 * i + 0] = tmpVerts[i][0];
    internalVertices[3 * i + 1] = tmpVerts[i][1];
    internalVertices[3 * i + 2] = tmpVerts[i][2];
  }

  for(usize i = 0; i < numInternalTris; i++)
  {
    internalTriangles[3 * i + 0] = tmpTris[i][0];
    internalTriangles[3 * i + 1] = tmpTris[i][1];
    internalTriangles[3 * i + 2] = tmpTris[i][2];
  }

  return {};
}
} // namespace complex
