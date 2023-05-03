#include "SurfaceNets.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include "ComplexCore/SurfaceNets/MMCellFlag.h"
#include "ComplexCore/SurfaceNets/MMCellMap.h"
#include "ComplexCore/SurfaceNets/MMSurfaceNet.h"

using namespace complex;

namespace
{
//
// Private data class for storing quad data during OBJ data generation
//
class MMQuad
{
public:
  MMQuad()
  : m_vertexIndices{-1, -1, -1, -1}
  , m_labels{0, 0}
  {
  }
  MMQuad(std::array<int32, 4> vi, std::array<int32, 2> labels)
  : m_vertexIndices{vi[0], vi[1], vi[2], vi[3]}
  , m_labels{labels[0], labels[1]}
  {
  }

  void getVertexIndices(int vertexIndices[4])
  {
    for(int i = 0; i < 4; i++)
      vertexIndices[i] = m_vertexIndices[i];
  }
  void getLabels(unsigned short labels[2])
  {
    for(int i = 0; i < 2; i++)
      labels[i] = m_labels[i];
  }
  void setVertexIndices(int vertexIndices[4])
  {
    for(int i = 0; i < 4; i++)
      m_vertexIndices[i] = vertexIndices[i];
  }
  void setLabels(unsigned short labels[2])
  {
    for(int i = 0; i < 2; i++)
      m_labels[i] = labels[i];
  }

private:
  int m_vertexIndices[4];
  int32_t m_labels[2];
};

struct vtxData
{
  int vID;
  float position[3];
};

void crossProduct(float v0[3], float v1[3], float result[3])
{
  // Cross product of vectors v0 and v1
  result[0] = v0[1] * v1[2] - v0[2] * v1[1];
  result[1] = v0[2] * v1[0] - v0[0] * v1[2];
  result[2] = v0[0] * v1[1] - v0[1] * v1[0];
}
float triangleArea(float p0[3], float p1[3], float p2[3])
{
  // Area of triangle with vertex positions p0, p1, p2
  float v01[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
  float v02[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
  float cp[3];
  crossProduct(v01, v02, cp);
  float magCP = std::sqrt(cp[0] * cp[0] + cp[1] * cp[1] + cp[2] * cp[2]);
  return 0.5 * magCP;
}

void getQuadTriangleIDs(vtxData vData[4], bool isQuadFrontFacing, int triangleVtxIDs[6])
{
  // Order quad vertices so quad is front facing
  if(!isQuadFrontFacing)
  {
    vtxData temp = vData[3];
    vData[3] = vData[1];
    vData[1] = temp;
  }

  // Order quad vertices so that the two generated triangles have the minimal area. This
  // reduces self intersections in the surface.
  float thisArea = triangleArea(vData[0].position, vData[1].position, vData[2].position) + triangleArea(vData[0].position, vData[2].position, vData[3].position);
  float alternateArea = triangleArea(vData[1].position, vData[2].position, vData[3].position) + triangleArea(vData[1].position, vData[3].position, vData[0].position);
  if(alternateArea < thisArea)
  {
    vtxData temp = vData[0];
    vData[0] = vData[1];
    vData[1] = vData[2];
    vData[2] = vData[3];
    vData[3] = temp;
  }

  // Generate vertex ids to triangulate the quad
  triangleVtxIDs[0] = vData[0].vID;
  triangleVtxIDs[1] = vData[1].vID;
  triangleVtxIDs[2] = vData[2].vID;
  triangleVtxIDs[3] = vData[0].vID;
  triangleVtxIDs[4] = vData[2].vID;
  triangleVtxIDs[5] = vData[3].vID;
}

} // namespace
// -----------------------------------------------------------------------------
SurfaceNets::SurfaceNets(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SurfaceNetsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SurfaceNets::~SurfaceNets() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& SurfaceNets::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> SurfaceNets::operator()()
{
  // Get the ImageGeometry
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GridGeomDataPath);

  // Get the Created Triangle Geometry
  TriangleGeom& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  auto gridDimensions = imageGeom.getDimensions();
  auto voxelSize = imageGeom.getSpacing();
  IntVec3 arraySize(static_cast<int32>(gridDimensions[0]), static_cast<int32>(gridDimensions[1]), static_cast<int32>(gridDimensions[2]));

  Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& featureIdDataStore = featureIds.getIDataStoreRefAs<Int32DataStore>();
  MMSurfaceNet surfaceNet(featureIdDataStore.data(), arraySize.data(), voxelSize.data());

  // Use current parameters to relax the SurfaceNet
  if(m_InputValues->ApplySmoothing)
  {
    MMSurfaceNet::RelaxAttrs m_relaxAttrs;
    m_relaxAttrs.maxDistFromCellCenter = m_InputValues->MaxDistanceFromVoxel;
    m_relaxAttrs.numRelaxIterations = m_InputValues->SmoothingIterations;
    m_relaxAttrs.relaxFactor = m_InputValues->RelaxationFactor;

    surfaceNet.relax(m_relaxAttrs);
  }

  auto* cellMap = surfaceNet.getCellMap();
  int nodeCount = cellMap->numVertices();
  triangleGeom.resizeVertexList(nodeCount);
  triangleGeom.getVertexAttributeMatrix()->resizeTuples({static_cast<usize>(nodeCount)});

  LinkedGeometryData& linkedGeometryData = triangleGeom.getLinkedGeometryData();

  // Remove and then insert a properly sized int8 for the NodeTypes
  m_DataStructure.removeData(m_InputValues->NodeTypesDataPath);
  Result<> nodeTypeResult = complex::CreateArray<int8_t>(m_DataStructure, {static_cast<usize>(nodeCount)}, {1}, m_InputValues->NodeTypesDataPath, IDataAction::Mode::Execute);
  Int8Array& nodeTypes = m_DataStructure.getDataRefAs<Int8Array>(m_InputValues->NodeTypesDataPath);
  linkedGeometryData.addVertexData(m_InputValues->NodeTypesDataPath);

  Point3D<float32> position = {0.0f, 0.0f, 0.0f};
  for(int32 vertIndex = 0; vertIndex < nodeCount; vertIndex++)
  {
    cellMap->getVertexPosition(vertIndex, position.data());
    triangleGeom.setVertexCoordinate(static_cast<usize>(vertIndex), position);
    nodeTypes[static_cast<usize>(vertIndex)] = static_cast<uint8>(cellMap->cellVertexType(vertIndex));
  }

  std::vector<MMQuad> m_quads;
  usize triangleCount = 0;
  // First Pass through to just count the number of triangles:
  for(int idxVtx = 0; idxVtx < nodeCount; idxVtx++)
  {
    std::array<int32, 4> vertexIndices = {0, 0, 0, 0};
    std::array<int32, 2> quadLabels = {0, 0};
    if(cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices.data(), quadLabels.data()))
    {
      triangleCount += 2;
    }
    if(cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices.data(), quadLabels.data()))
    {
      triangleCount += 2;
    }
    if(cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices.data(), quadLabels.data()))
    {
      triangleCount += 2;
    }
  }
  triangleGeom.resizeFaceList(triangleCount);
  triangleGeom.getFaceAttributeMatrix()->resizeTuples({triangleCount});

  // Remove and then insert a properly sized Int32Array for the FaceLabels
  m_DataStructure.removeData(m_InputValues->FaceLabelsDataPath);
  Result<> faceLabelResult = complex::CreateArray<int32_t>(m_DataStructure, {triangleCount}, {2}, m_InputValues->FaceLabelsDataPath, IDataAction::Mode::Execute);
  Int32Array& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FaceLabelsDataPath);
  linkedGeometryData.addFaceData(m_InputValues->FaceLabelsDataPath);

  usize faceIndex = 0;
  //   Create temporary storage for cell quads which are constructed around edges
  //   crossed by the surface. Handle 3 edges per cell. The other 9 cell edges will
  //   be handled when neighboring cells that share edges with this cell are visited.
  std::array<usize, 3> t1;
  std::array<usize, 3> t2;
  std::array<int, 6> triangleVtxIDs;
  std::array<int32, 4> vertexIndices = {0, 0, 0, 0};
  std::array<int32, 2> quadLabels = {0, 0};
  bool isQuadFrontFacing = false;
  for(int idxVtx = 0; idxVtx < nodeCount; idxVtx++)
  {

    // Back-bottom edge
    if(cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices.data(), quadLabels.data()))
    {
      vtxData vData[4];
      for(int i = 0; i < 4; i++)
      {
        vData[i] = {vertexIndices[i], 00.0f, 0.0f, 0.0f};
      }

      isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);

      getQuadTriangleIDs(vData, isQuadFrontFacing, triangleVtxIDs.data());
      t1 = {static_cast<usize>(triangleVtxIDs[0]), static_cast<usize>(triangleVtxIDs[1]), static_cast<usize>(triangleVtxIDs[2])};
      t2 = {static_cast<usize>(triangleVtxIDs[3]), static_cast<usize>(triangleVtxIDs[4]), static_cast<usize>(triangleVtxIDs[5])};

      triangleGeom.setFacePointIds(faceIndex, t1);
      faceLabels[faceIndex] = quadLabels[0];
      faceIndex++;
      triangleGeom.setFacePointIds(faceIndex, t2);
      faceLabels[faceIndex] = quadLabels[1];
      faceIndex++;
    }

    // Left-bottom edge
    if(cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices.data(), quadLabels.data()))
    {
      vtxData vData[4];
      for(int i = 0; i < 4; i++)
      {
        vData[i] = {vertexIndices[i], 00.0f, 0.0f, 0.0f};
      }

      isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);

      getQuadTriangleIDs(vData, isQuadFrontFacing, triangleVtxIDs.data());
      t1 = {static_cast<usize>(triangleVtxIDs[0]), static_cast<usize>(triangleVtxIDs[1]), static_cast<usize>(triangleVtxIDs[2])};
      t2 = {static_cast<usize>(triangleVtxIDs[3]), static_cast<usize>(triangleVtxIDs[4]), static_cast<usize>(triangleVtxIDs[5])};

      triangleGeom.setFacePointIds(faceIndex, t1);
      faceLabels[faceIndex] = quadLabels[0];
      faceIndex++;
      triangleGeom.setFacePointIds(faceIndex, t2);
      faceLabels[faceIndex] = quadLabels[1];
      faceIndex++;
    }

    // Left-back edge
    if(cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices.data(), quadLabels.data()))
    {
      vtxData vData[4];
      for(int i = 0; i < 4; i++)
      {
        vData[i] = {vertexIndices[i], 00.0f, 0.0f, 0.0f};
      }

      isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);

      getQuadTriangleIDs(vData, isQuadFrontFacing, triangleVtxIDs.data());
      t1 = {static_cast<usize>(triangleVtxIDs[0]), static_cast<usize>(triangleVtxIDs[1]), static_cast<usize>(triangleVtxIDs[2])};
      t2 = {static_cast<usize>(triangleVtxIDs[3]), static_cast<usize>(triangleVtxIDs[4]), static_cast<usize>(triangleVtxIDs[5])};

      triangleGeom.setFacePointIds(faceIndex, t1);
      faceLabels[faceIndex] = quadLabels[0];
      faceIndex++;
      triangleGeom.setFacePointIds(faceIndex, t2);
      faceLabels[faceIndex] = quadLabels[1];
      faceIndex++;
    }
  }

  // Now Extract the Triangles from the SurfaceNets object.
  return {};
}
