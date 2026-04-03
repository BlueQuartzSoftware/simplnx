#include "ExtractInternalSurfacesFromTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <limits>
#include <unordered_map>

using namespace nx::core;

namespace
{

struct RemoveFlaggedVerticesFunctor
{
  // copy data to masked geometry
  template <class T>
  void operator()(IDataArray* inputDataPtr, IDataArray* outputDataArray, const std::vector<IGeometry::MeshIndexType>& indexMapping) const
  {
    auto& inputData = inputDataPtr->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& outputData = outputDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
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

// -----------------------------------------------------------------------------
ExtractInternalSurfacesFromTriangleGeometry::ExtractInternalSurfacesFromTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                         ExtractInternalSurfacesFromTriangleGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExtractInternalSurfacesFromTriangleGeometry::~ExtractInternalSurfacesFromTriangleGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ExtractInternalSurfacesFromTriangleGeometry::operator()()
{
  // auto nodeTypesArrayPath = filterArgs.value<DataPath>(k_NodeTypesPath_Key);
  // auto triangleGeomPath = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  auto internalTrianglesPath = m_InputValues->OutputTriangleGeometryPath;
  // auto copyVertexPaths = filterArgs.value<std::vector<DataPath>>(k_CopyVertexPaths_Key);
  // auto copyTrianglePaths = filterArgs.value<std::vector<DataPath>>(k_CopyTrianglePaths_Key);
  // auto vertexDataName = filterArgs.value<std::string>(k_VertexAttributeMatrixName_Key);
  // auto faceDataName = filterArgs.value<std::string>(k_TriangleAttributeMatrixName_Key);
  auto minMaxNodeValues = m_InputValues->NodeTypeRange;

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->InputTriangleGeometryPath);
  auto& internalTriangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(internalTrianglesPath);
  auto& vertices = *triangleGeom.getVertices();
  auto& triangles = *triangleGeom.getFaces();
  auto numVerts = triangleGeom.getNumberOfVertices();
  auto numTris = triangleGeom.getNumberOfFaces();

  auto& nodeTypes = m_DataStructure.getDataRefAs<Int8Array>(m_InputValues->NodeTypesPath);

  auto internalVerticesPath = internalTrianglesPath.createChildPath(TriangleGeom::k_SharedVertexListName);
  internalTriangleGeom.setVertices(*m_DataStructure.getDataAs<Float32Array>(internalVerticesPath));

  auto internalFacesPath = internalTrianglesPath.createChildPath(TriangleGeom::k_SharedFacesListName);
  internalTriangleGeom.setFaceList(*m_DataStructure.getDataAs<UInt64Array>(internalFacesPath));

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

  MessageHelper messageHelper(m_MessageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(numTris);
  progressHelper.setProgressMessageTemplate("ExtractInternalSurfaces: {:.1f}% Complete");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  // Loop over all the triangles mapping the triangle and the vertices to the new array locations
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

    progressMessenger.sendProgressMessage(1);

    if(m_ShouldCancel)
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
    if(m_ShouldCancel)
    {
      return {};
    }
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
    if(m_ShouldCancel)
    {
      return {};
    }
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
  for(const auto& targetArrayPath : m_InputValues->CopyVertexArrayPaths)
  {
    DataPath destinationPath = internalTrianglesPath.createChildPath(m_InputValues->VertexAttributeMatrixName).createChildPath(targetArrayPath.getTargetName());
    auto* src = m_DataStructure.getDataAs<IDataArray>(targetArrayPath);
    auto* dest = m_DataStructure.getDataAs<IDataArray>(destinationPath);

    ExecuteDataFunction(RemoveFlaggedVerticesFunctor{}, src->getDataType(), src, dest, vertNewIndex);
  }

  for(const auto& targetArrayPath : m_InputValues->CopyTriangleArrayPaths)
  {
    DataPath destinationPath = internalTrianglesPath.createChildPath(m_InputValues->TriangleAttributeMatrixName).createChildPath(targetArrayPath.getTargetName());
    auto* src = m_DataStructure.getDataAs<IDataArray>(targetArrayPath);
    auto* dest = m_DataStructure.getDataAs<IDataArray>(destinationPath);
    dest->resizeTuples({currentNewTriIndex});

    ExecuteDataFunction(RemoveFlaggedVerticesFunctor{}, src->getDataType(), src, dest, triNewIndex);
  }

  return {};
}
