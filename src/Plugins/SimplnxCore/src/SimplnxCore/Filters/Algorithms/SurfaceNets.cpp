#include "SurfaceNets.hpp"
#include "TupleTransfer.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "SimplnxCore/SurfaceNets/MMCellFlag.h"
#include "SimplnxCore/SurfaceNets/MMCellMap.h"
#include "SimplnxCore/SurfaceNets/MMGeometryOBJ.h"
#include "SimplnxCore/SurfaceNets/MMSurfaceNet.h"

using namespace nx::core;

namespace
{
struct VertexData
{
  int VertexId;
  std::array<float32, 3> Position;
};

void crossProduct(const std::array<float32, 3>& vert0, const std::array<float32, 3> vert1, std::array<float32, 3> result)
{
  // Cross product of vectors v0 and v1
  result[0] = vert0[1] * vert1[2] - vert0[2] * vert1[1];
  result[1] = vert0[2] * vert1[0] - vert0[0] * vert1[2];
  result[2] = vert0[0] * vert1[1] - vert0[1] * vert1[0];
}
float triangleArea(std::array<float32, 3>& vert0, std::array<float32, 3>& vert1, std::array<float32, 3>& vert2)
{
  // Area of triangle with vertex positions p0, p1, p2
  const std::array<float32, 3> v01 = {vert1[0] - vert0[0], vert1[1] - vert0[1], vert1[2] - vert0[2]};
  const std::array<float32, 3> v02 = {vert2[0] - vert0[0], vert2[1] - vert0[1], vert2[2] - vert0[2]};
  std::array<float32, 3> cross = {0.0f, 0.0f, 0.0f};
  crossProduct(v01, v02, cross);
  float const magCP = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
  return 0.5f * magCP;
}

void getQuadTriangleIDs(std::array<VertexData, 4>& vData, bool isQuadFrontFacing, std::array<int32, 6>& triangleVtxIDs)
{
  // Order quad vertices so quad is front facing
  if(!isQuadFrontFacing)
  {
    VertexData const temp = vData[3];
    vData[3] = vData[1];
    vData[1] = temp;
  }

  // Order quad vertices so that the two generated triangles have the minimal area. This
  // reduces self intersections in the surface.
  float const thisArea = triangleArea(vData[0].Position, vData[1].Position, vData[2].Position) + triangleArea(vData[0].Position, vData[2].Position, vData[3].Position);
  float const alternateArea = triangleArea(vData[1].Position, vData[2].Position, vData[3].Position) + triangleArea(vData[1].Position, vData[3].Position, vData[0].Position);
  if(alternateArea < thisArea)
  {
    VertexData const temp = vData[0];
    vData[0] = vData[1];
    vData[1] = vData[2];
    vData[2] = vData[3];
    vData[3] = temp;
  }

  // Generate vertex ids to triangulate the quad
  triangleVtxIDs[0] = vData[0].VertexId;
  triangleVtxIDs[1] = vData[1].VertexId;
  triangleVtxIDs[2] = vData[2].VertexId;
  triangleVtxIDs[3] = vData[0].VertexId;
  triangleVtxIDs[4] = vData[2].VertexId;
  triangleVtxIDs[5] = vData[3].VertexId;
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
  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  auto gridDimensions = imageGeom.getDimensions();
  auto voxelSize = imageGeom.getSpacing();
  auto origin = imageGeom.getOrigin();

  // X Y Z Ordering
  IntVec3 arraySize(static_cast<int32>(gridDimensions[0]), static_cast<int32>(gridDimensions[1]), static_cast<int32>(gridDimensions[2]));

  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  // THIS IS MAKING A COPY OF THE IMAGE GEOMETRY CELL "FEATURE IDS" array
  using LabelType = int32_t;
  std::vector<LabelType> labels(featureIds.getNumberOfTuples());
  for(size_t idx = 0; idx < featureIds.getNumberOfTuples(); idx++)
  {
    labels[idx] = static_cast<LabelType>(featureIds[idx]);
  }

  MMSurfaceNet surfaceNet(labels.data(), arraySize.data(), voxelSize.data());
  labels.resize(0); // Free that copy... because there is another copy in the surfaceNet object.

  // Use current parameters to relax the SurfaceNet
  if(m_InputValues->ApplySmoothing)
  {
    MMSurfaceNet::RelaxAttrs relaxAttrs{};
    relaxAttrs.maxDistFromCellCenter = m_InputValues->MaxDistanceFromVoxel;
    relaxAttrs.numRelaxIterations = m_InputValues->SmoothingIterations;
    relaxAttrs.relaxFactor = m_InputValues->RelaxationFactor;

    surfaceNet.relax(relaxAttrs);
  }

  auto* cellMapPtr = surfaceNet.getCellMap();
  const int nodeCount = cellMapPtr->numVertices();

  std::array<int, 3> arraySize2 = {0, 0, 0};
  cellMapPtr->getArraySize(arraySize2.data());

  triangleGeom.resizeVertexList(nodeCount);
  triangleGeom.getVertexAttributeMatrix()->resizeTuples({static_cast<usize>(nodeCount)});

  // Remove and then insert a properly sized int8 for the NodeTypes
  auto& nodeTypes = m_DataStructure.getDataAs<Int8Array>(m_InputValues->NodeTypesDataPath)->getDataStoreRef();
  nodeTypes.resizeTuples({static_cast<usize>(nodeCount)});

  Point3Df position = {0.0f, 0.0f, 0.0f};

  std::array<int, 3> vertCellIndex = {0, 0, 0};
  for(int32 vertIndex = 0; vertIndex < nodeCount; vertIndex++)
  {
    cellMapPtr->getVertexPosition(vertIndex, position.data());
    // Relocate the vertex correctly based on the origin of the ImageGeometry
    position = position + origin - Point3Df(0.5f * voxelSize[0], 0.5f * voxelSize[1], 0.5f * voxelSize[1]);

    triangleGeom.setVertexCoordinate(static_cast<usize>(vertIndex), position);
    cellMapPtr->getVertexCellIndex(vertIndex, vertCellIndex.data());
    MMCellMap::Cell* currentCellPtr = cellMapPtr->getCell(vertCellIndex.data());
    nodeTypes[static_cast<usize>(vertIndex)] = static_cast<int8>(currentCellPtr->flag.numJunctions());
  }
  usize triangleCount = 0;
  std::array<ptrdiff_t, 2> cellDataIndex = {0, 0};
  // First Pass through to just count the number of triangles:
  for(int idxVtx = 0; idxVtx < nodeCount; idxVtx++)
  {
    std::array<int32, 4> vertexIndices = {0, 0, 0, 0};
    std::array<LabelType, 2> quadLabels = {0, 0};
    std::array<int32_t, 3> cellIndices = {0, 0, 0};

    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices.data(), quadLabels.data(), cellIndices))
    {
      if(quadLabels[0] == MMSurfaceNet::Padding || quadLabels[1] == MMSurfaceNet::Padding)
      {
        for(auto& vertIndex : vertexIndices)
        {
          if(nodeTypes[static_cast<usize>(vertIndex)] < 10)
          {
            nodeTypes[static_cast<usize>(vertIndex)] += 10;
          }
          else
          {
            nodeTypes[static_cast<usize>(vertIndex)] += 1;
          }
        }
      }
      triangleCount += 2;
    }
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices.data(), quadLabels.data(), cellIndices))
    {
      if(quadLabels[0] == MMSurfaceNet::Padding || quadLabels[1] == MMSurfaceNet::Padding)
      {
        for(auto& vertIndex : vertexIndices)
        {
          if(nodeTypes[static_cast<usize>(vertIndex)] < 10)
          {
            nodeTypes[static_cast<usize>(vertIndex)] += 10;
          }
          else
          {
            nodeTypes[static_cast<usize>(vertIndex)] += 1;
          }
        }
      }
      triangleCount += 2;
    }
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices.data(), quadLabels.data(), cellIndices))
    {
      if(quadLabels[0] == MMSurfaceNet::Padding || quadLabels[1] == MMSurfaceNet::Padding)
      {
        for(auto& vertIndex : vertexIndices)
        {
          if(nodeTypes[static_cast<usize>(vertIndex)] < 10)
          {
            nodeTypes[static_cast<usize>(vertIndex)] += 10;
          }
          else
          {
            nodeTypes[static_cast<usize>(vertIndex)] += 1;
          }
        }
      }
      triangleCount += 2;
    }
  }
  triangleGeom.resizeFaceList(triangleCount);
  triangleGeom.getFaceAttributeMatrix()->resizeTuples({triangleCount});

  // Resize the face labels Int32Array
  auto& faceLabels = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef();
  faceLabels.resizeTuples({triangleCount});
  faceLabels.fill(-1000);

  // Create a vector of TupleTransferFunctions for each of the Triangle Face to VertexType Data Arrays
  std::vector<std::shared_ptr<AbstractTupleTransfer>> tupleTransferFunctions;
  for(size_t i = 0; i < m_InputValues->SelectedDataArrayPaths.size(); i++)
  {
    // Associate these arrays with the Triangle Face Data.
    ::AddTupleTransferInstance(m_DataStructure, m_InputValues->SelectedDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i], tupleTransferFunctions);
  }

  usize faceIndex = 0;
  //   Create temporary storage for cell quads which are constructed around edges
  //   crossed by the surface. Handle 3 edges per cell. The other 9 cell edges will
  //   be handled when neighboring cells that share edges with this cell are visited.
  std::array<usize, 3> t1 = {0, 0, 0};
  std::array<usize, 3> t2 = {0, 0, 0};
  std::array<int, 6> triangleVtxIDs = {0, 0, 0, 0, 0, 0};
  std::array<int32, 4> vertexIndices = {0, 0, 0, 0};
  std::array<LabelType, 2> quadLabels = {0, 0};
  std::array<VertexData, 4> vData{};
  std::array<int32_t, 3> cellIndices = {0, 0, 0};

  constexpr size_t x_idx = 0;
  constexpr size_t y_idx = 1;
  constexpr size_t z_idx = 2;

  for(int idxVtx = 0; idxVtx < nodeCount; idxVtx++)
  {

    //        std::array<int32_t, 3> cellIndex;
    //        cellMapPtr->getVertexCellIndex(idxVtx, cellIndex.data());
    //        std::cout << cellIndex[0] << ", " << cellIndex[1] << ", " << cellIndex[2] << std::endl;

    GenerateTriangles(idxVtx, MMCellFlag::Edge::BackBottomEdge, cellMapPtr, imageGeom, triangleGeom, faceIndex, faceLabels, featureIds, tupleTransferFunctions);
    GenerateTriangles(idxVtx, MMCellFlag::Edge::LeftBottomEdge, cellMapPtr, imageGeom, triangleGeom, faceIndex, faceLabels, featureIds, tupleTransferFunctions);
    GenerateTriangles(idxVtx, MMCellFlag::Edge::LeftBackEdge, cellMapPtr, imageGeom, triangleGeom, faceIndex, faceLabels, featureIds, tupleTransferFunctions);

#if 0
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices.data(), quadLabels.data(), cellIndices))
    {
      size_t cellIndex1 = cellIndices[z_idx] * gridDimensions[y_idx] * gridDimensions[x_idx] + cellIndices[y_idx] * gridDimensions[x_idx] + cellIndices[x_idx];
      size_t cellIndex2 = (cellIndices[z_idx] - 1) * gridDimensions[y_idx] * gridDimensions[x_idx] + cellIndices[y_idx] * gridDimensions[x_idx] + cellIndices[x_idx];

      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);

      getQuadTriangleIDs(vData, isQuadFrontFacing, triangleVtxIDs);
      t1 = {static_cast<usize>(triangleVtxIDs[0]), static_cast<usize>(triangleVtxIDs[1]), static_cast<usize>(triangleVtxIDs[2])};
      t2 = {static_cast<usize>(triangleVtxIDs[3]), static_cast<usize>(triangleVtxIDs[4]), static_cast<usize>(triangleVtxIDs[5])};

      triangleGeom.setFacePointIds(faceIndex, t1);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabels[faceIndex * 2] = quadLabels[0];
        faceLabels[faceIndex * 2 + 1] = quadLabels[1];
      }
      else
      {
        faceLabels[faceIndex * 2] = quadLabels[1];
        faceLabels[faceIndex * 2 + 1] = quadLabels[0];
      }
      // Copy any Cell Data to the Triangle Mesh
      for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
      {
        tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, cellIndex1, cellIndex2);
      }

      faceIndex++;

      triangleGeom.setFacePointIds(faceIndex, t2);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabels[faceIndex * 2] = quadLabels[0];
        faceLabels[faceIndex * 2 + 1] = quadLabels[1];
      }
      else
      {
        faceLabels[faceIndex * 2] = quadLabels[1];
        faceLabels[faceIndex * 2 + 1] = quadLabels[0];
      }
      // Copy any Cell Data to the Triangle Mesh
      for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
      {
        tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, cellIndex1, cellIndex2);
      }
      faceIndex++;
    }

    // Left-bottom edge Y
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices.data(), quadLabels.data(), cellIndices))
    {
      size_t cellIndex1 = cellIndices[z_idx] * gridDimensions[y_idx] * gridDimensions[x_idx] + cellIndices[y_idx] * gridDimensions[x_idx] + cellIndices[x_idx];
      size_t cellIndex2 = cellIndices[z_idx] * (gridDimensions[y_idx] - 1) * gridDimensions[x_idx] + (cellIndices[y_idx] - 1) * gridDimensions[x_idx] + cellIndices[x_idx];

      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);

      getQuadTriangleIDs(vData, isQuadFrontFacing, triangleVtxIDs);
      t1 = {static_cast<usize>(triangleVtxIDs[0]), static_cast<usize>(triangleVtxIDs[1]), static_cast<usize>(triangleVtxIDs[2])};
      t2 = {static_cast<usize>(triangleVtxIDs[3]), static_cast<usize>(triangleVtxIDs[4]), static_cast<usize>(triangleVtxIDs[5])};

      triangleGeom.setFacePointIds(faceIndex, t1);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabels[faceIndex * 2] = quadLabels[0];
        faceLabels[faceIndex * 2 + 1] = quadLabels[1];
      }
      else
      {
        faceLabels[faceIndex * 2] = quadLabels[1];
        faceLabels[faceIndex * 2 + 1] = quadLabels[0];
      }
      // Copy any Cell Data to the Triangle Mesh
      for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
      {
        tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, cellIndex1, cellIndex2);
      }
      faceIndex++;

      triangleGeom.setFacePointIds(faceIndex, t2);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabels[faceIndex * 2] = quadLabels[0];
        faceLabels[faceIndex * 2 + 1] = quadLabels[1];
      }
      else
      {
        faceLabels[faceIndex * 2] = quadLabels[1];
        faceLabels[faceIndex * 2 + 1] = quadLabels[0];
      }
      // Copy any Cell Data to the Triangle Mesh
      for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
      {
        tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, cellIndex1, cellIndex2);
      }
      faceIndex++;
    }

    // Left-back edge X
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices.data(), quadLabels.data(), cellIndices))
    {
      size_t cellIndex1 = cellIndices[z_idx] * gridDimensions[y_idx] * gridDimensions[x_idx] + cellIndices[y_idx] * gridDimensions[x_idx] + cellIndices[x_idx];
      size_t cellIndex2 = cellIndices[z_idx] * gridDimensions[y_idx] * gridDimensions[x_idx] + cellIndices[y_idx] * gridDimensions[x_idx] + (cellIndices[x_idx] - 1);

      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);

      getQuadTriangleIDs(vData, isQuadFrontFacing, triangleVtxIDs);
      t1 = {static_cast<usize>(triangleVtxIDs[0]), static_cast<usize>(triangleVtxIDs[1]), static_cast<usize>(triangleVtxIDs[2])};
      t2 = {static_cast<usize>(triangleVtxIDs[3]), static_cast<usize>(triangleVtxIDs[4]), static_cast<usize>(triangleVtxIDs[5])};

      triangleGeom.setFacePointIds(faceIndex, t1);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabels[faceIndex * 2] = quadLabels[0];
        faceLabels[faceIndex * 2 + 1] = quadLabels[1];
      }
      else
      {
        faceLabels[faceIndex * 2] = quadLabels[1];
        faceLabels[faceIndex * 2 + 1] = quadLabels[0];
      }
      // Copy any Cell Data to the Triangle Mesh
      for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
      {
        tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, cellIndex1, cellIndex2);
      }
      faceIndex++;

      triangleGeom.setFacePointIds(faceIndex, t2);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabels[faceIndex * 2] = quadLabels[0];
        faceLabels[faceIndex * 2 + 1] = quadLabels[1];
      }
      else
      {
        faceLabels[faceIndex * 2] = quadLabels[1];
        faceLabels[faceIndex * 2 + 1] = quadLabels[0];
      }
      // Copy any Cell Data to the Triangle Mesh
      for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
      {
        tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, cellIndex1, cellIndex2);
      }
      faceIndex++;
    }
#endif
  }

  return {};
}

using LabelType = int32;

/**
 *
 * @param idxVtx
 * @param edgeType
 * @param cellMapPtr
 * @param imageGeom
 * @param triangleGeom
 * @param faceIndex
 * @param faceLabels
 * @param tupleTransferFunctions
 */
void SurfaceNets::GenerateTriangles(int idxVtx, MMCellFlag::Edge edgeType, MMCellMap* cellMapPtr, const ImageGeom& imageGeom, TriangleGeom& triangleGeom, usize& faceIndex,
                                    AbstractDataStore<int32_t>& faceLabels, AbstractDataStore<int32_t>& featureIds, std::vector<std::shared_ptr<AbstractTupleTransfer>>& tupleTransferFunctions)
{
  auto gridDimensions = imageGeom.getDimensions();

  std::array<int32, 4> vertexIndices = {0, 0, 0, 0};
  std::array<LabelType, 2> quadLabels = {0, 0};
  std::array<int32_t, 3> cellIndex = {0, 0, 0};
  if(!cellMapPtr->getEdgeQuad(idxVtx, edgeType, vertexIndices.data(), quadLabels.data(), cellIndex))
  {
    return;
  }
  constexpr size_t x_idx = 0;
  constexpr size_t y_idx = 1;
  constexpr size_t z_idx = 2;

  //  cellIndex[x_idx] = (cellIndex[x_idx] > 0 ? cellIndex[x_idx] - 1 : cellIndex[x_idx]);
  //  cellIndex[y_idx] = (cellIndex[y_idx] > 0 ? cellIndex[y_idx] - 1 : cellIndex[y_idx]);
  //  cellIndex[z_idx] = (cellIndex[z_idx] > 0 ? cellIndex[z_idx] - 1 : cellIndex[z_idx]);

  std::array<size_t, 2> cellOffset = {(cellIndex[z_idx] * gridDimensions[y_idx] * gridDimensions[x_idx]) + (cellIndex[y_idx] * gridDimensions[x_idx]) + cellIndex[x_idx], 0};

  if(edgeType == MMCellFlag::Edge::LeftBottomEdge)
  {
    cellOffset[1] = cellOffset[0] + gridDimensions[x_idx];
  }
  else if(edgeType == MMCellFlag::Edge::BackBottomEdge)
  {
    cellOffset[1] = cellOffset[0] + 1;
  }
  else if(edgeType == MMCellFlag::Edge::LeftBackEdge)
  {
    cellOffset[1] = cellOffset[0] + gridDimensions[x_idx] * gridDimensions[y_idx];
  }
  else
  {
    throw std::runtime_error("SurfaceNets: GenerateTriangles used unknown enumeration for edge type");
  }

  std::array<VertexData, 4> vData = {VertexData{vertexIndices[0], 00.0f, 0.0f, 0.0f}, VertexData{vertexIndices[1], 00.0f, 0.0f, 0.0f}, VertexData{vertexIndices[2], 00.0f, 0.0f, 0.0f},
                                     VertexData{vertexIndices[3], 00.0f, 0.0f, 0.0f}};

  const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);

  std::array<int, 6> triangleVtxIDs = {0, 0, 0, 0, 0, 0};
  getQuadTriangleIDs(vData, isQuadFrontFacing, triangleVtxIDs);
  std::array<usize, 3> t1 = {static_cast<usize>(triangleVtxIDs[0]), static_cast<usize>(triangleVtxIDs[1]), static_cast<usize>(triangleVtxIDs[2])};
  triangleGeom.setFacePointIds(faceIndex, t1);
  std::array<usize, 3> t2 = {static_cast<usize>(triangleVtxIDs[3]), static_cast<usize>(triangleVtxIDs[4]), static_cast<usize>(triangleVtxIDs[5])};
  triangleGeom.setFacePointIds(faceIndex + 1, t2);

  if(quadLabels[0] != -1 && quadLabels[1] != -1)
  {
    size_t firstIdx = 0;
    size_t secIdx = 1;
    if(quadLabels[1] < quadLabels[0])
    {
      firstIdx = 1;
      secIdx = 0;
    }

    std::cout << quadLabels[firstIdx] << "\t" << quadLabels[secIdx] << "\t" << featureIds[cellOffset[firstIdx]] << '\t' << featureIds[cellOffset[secIdx]] << "\n";

    faceLabels[faceIndex * 2] = (quadLabels[firstIdx] == -1 ? -1 : faceLabels[cellOffset[firstIdx]]); // quadLabels[firstIdx];
    faceLabels[faceIndex * 2 + 1] = (quadLabels[secIdx] == -1 ? -1 : faceLabels[cellOffset[secIdx]]); // quadLabels[firstIdx];

    faceLabels[(faceIndex + 1) * 2] = (quadLabels[firstIdx] == -1 ? -1 : faceLabels[cellOffset[firstIdx]]); // quadLabels[firstIdx];
    faceLabels[(faceIndex + 1) * 2 + 1] = (quadLabels[secIdx] == -1 ? -1 : faceLabels[cellOffset[secIdx]]); // quadLabels[firstIdx];
  }
  else // One of the Quad Labels is -1, which means we are on the virtual border
  {
    // Assume the first Quad Label = -1
    size_t firstIdx = 0;
    size_t secIdx = 1;
    if(quadLabels[1] == -1)
    {
      firstIdx = 1;
      secIdx = 0;
    }

    std::cout << quadLabels[firstIdx] << "\t" << quadLabels[secIdx] << "\t" << featureIds[cellOffset[firstIdx]] << '\t' << featureIds[cellOffset[secIdx]] << "\n";

    if(quadLabels[secIdx] != -1 && quadLabels[secIdx] != featureIds[cellOffset[secIdx]])
    {
      std::cout << "   " << cellIndex[0] << "\t" << cellIndex[1] << "\t" << cellIndex[2] << "\n";
    }

    faceLabels[faceIndex * 2] = (quadLabels[firstIdx] == -1 ? -666 : faceLabels[cellOffset[firstIdx]]); // quadLabels[firstIdx];
    faceLabels[faceIndex * 2 + 1] = (quadLabels[secIdx] == -1 ? -1 : faceLabels[cellOffset[secIdx]]);   // quadLabels[firstIdx];

    faceLabels[(faceIndex + 1) * 2] = (quadLabels[firstIdx] == -1 ? -666 : faceLabels[cellOffset[firstIdx]]); // quadLabels[firstIdx];
    faceLabels[(faceIndex + 1) * 2 + 1] = (quadLabels[secIdx] == -1 ? -1 : faceLabels[cellOffset[secIdx]]);   // quadLabels[firstIdx];

    // Copy any Cell Data to the Triangle Mesh for the first triangle
    for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
    {
      tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, cellOffset[firstIdx], cellOffset[secIdx], faceLabels);
    }
    // Copy any Cell Data to the Triangle Mesh for the second triangle
    for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
    {
      tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex + 1, cellOffset[firstIdx], cellOffset[secIdx], faceLabels);
    }
  }

  // Increment the 'faceIndex' index by 2
  faceIndex = faceIndex + 2;
}
