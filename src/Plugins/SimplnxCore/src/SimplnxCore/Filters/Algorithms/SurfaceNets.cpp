#include "SurfaceNets.hpp"
#include "TupleTransfer.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include "SimplnxCore/SurfaceNets/MMCellFlag.h"
#include "SimplnxCore/SurfaceNets/MMCellMap.h"
#include "SimplnxCore/SurfaceNets/MMGeometryOBJ.h"
#include "SimplnxCore/SurfaceNets/MMSurfaceNet.h"

using namespace nx::core;

namespace
{
using LabelType = int32;
constexpr inline int8 CalculatePadding(int8 value)
{
  return value + ((9 * static_cast<int8>(value < 10)) + 1);
}

inline void HandlePadding(std::array<size_t, 4> vertexIndices, AbstractDataStore<int8>& nodeTypes)
{
  nodeTypes.setValue(vertexIndices[0], CalculatePadding(nodeTypes.getValue(vertexIndices[0])));
  nodeTypes.setValue(vertexIndices[1], CalculatePadding(nodeTypes.getValue(vertexIndices[1])));
  nodeTypes.setValue(vertexIndices[2], CalculatePadding(nodeTypes.getValue(vertexIndices[2])));
  nodeTypes.setValue(vertexIndices[3], CalculatePadding(nodeTypes.getValue(vertexIndices[3])));
};

struct VertexData
{
  size_t VertexId;
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

void getQuadTriangleIDs(std::array<VertexData, 4>& vData, bool isQuadFrontFacing, std::array<size_t, 6>& triangleVtxIDs)
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
  auto* triangleGeomPtr = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  auto gridDimensions = imageGeom.getDimensions();
  auto voxelSize = imageGeom.getSpacing();
  auto origin = imageGeom.getOrigin();

  MMSurfaceNet surfaceNet(triangleGeomPtr->getVerticesRef().getDataStoreRef(), m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath), gridDimensions.data(), voxelSize.data());
  if(!surfaceNet.getCellMap()->valid())
  {
    return MakeErrorResult(-843870, fmt::format("Could not allocate SurfaceNets internal Data Structures"));
  }

  // Use current parameters to relax the SurfaceNet
  if(m_InputValues->ApplySmoothing)
  {
    MMSurfaceNet::RelaxAttrs relaxAttrs{};
    relaxAttrs.maxDistFromCellCenter = m_InputValues->MaxDistanceFromVoxel;
    relaxAttrs.numRelaxIterations = m_InputValues->SmoothingIterations;
    relaxAttrs.relaxFactor = m_InputValues->RelaxationFactor;

    surfaceNet.relax(relaxAttrs);
  }

  auto cellMapPtr = surfaceNet.getCellMap();
  const size_t nodeCount = cellMapPtr->numVertices();

  std::array<int, 3> arraySize2 = {0, 0, 0};
  cellMapPtr->getArraySize(arraySize2.data());

  triangleGeom.getVertexAttributeMatrix()->resizeTuples({static_cast<usize>(nodeCount)});

  // Remove and then insert a properly sized int8 for the NodeTypes
  auto& nodeTypes = m_DataStructure.getDataAs<Int8Array>(m_InputValues->NodeTypesDataPath)->getDataStoreRef();
  nodeTypes.resizeTuples({static_cast<usize>(nodeCount)});

  Point3Df position = {0.0f, 0.0f, 0.0f};

  std::array<int, 3> vertCellIndex = {0, 0, 0};
  for(size_t vertIndex = 0; vertIndex < nodeCount; vertIndex++)
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
  std::array<usize, 2> quadNxArrayIndices = {0, 0};
  // First Pass through to just count the number of triangles:
  for(int idxVtx = 0; idxVtx < nodeCount; idxVtx++)
  {
    std::array<size_t, 4> vertexIndices = {0, 0, 0, 0};
    std::array<::LabelType, 2> quadLabels = {0, 0};

    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()))
    {
      if(quadLabels[0] == MMSurfaceNet::Padding || quadLabels[1] == MMSurfaceNet::Padding)
      {
        HandlePadding(vertexIndices, nodeTypes);
      }
      triangleCount += 2;
    }
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()))
    {
      if(quadLabels[0] == MMSurfaceNet::Padding || quadLabels[1] == MMSurfaceNet::Padding)
      {
        HandlePadding(vertexIndices, nodeTypes);
      }
      triangleCount += 2;
    }
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()))
    {
      if(quadLabels[0] == MMSurfaceNet::Padding || quadLabels[1] == MMSurfaceNet::Padding)
      {
        HandlePadding(vertexIndices, nodeTypes);
      }
      triangleCount += 2;
    }
  }

  triangleGeom.resizeFaceList(triangleCount);
  triangleGeom.getFaceAttributeMatrix()->resizeTuples({triangleCount});

  // Resize the face labels Int32Array
  auto& faceLabels = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef();
  faceLabels.resizeTuples({triangleCount});

  // Create a vector of TupleTransferFunctions for each of the Triangle Face
  std::vector<std::shared_ptr<AbstractTupleTransfer>> tupleTransferFunctions;
  for(size_t i = 0; i < m_InputValues->SelectedCellDataArrayPaths.size(); i++)
  {
    // Associate these arrays with the Triangle Face Data.
    ::AddTupleTransferInstance(m_DataStructure, m_InputValues->SelectedCellDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i], tupleTransferFunctions);
  }

  auto numSelectedCellArrayPaths = m_InputValues->SelectedCellDataArrayPaths.size();

  for(size_t i = 0; i < m_InputValues->SelectedFeatureDataArrayPaths.size(); i++)
  {
    // Associate these arrays with the Triangle Face Data.
    auto selectedPath = m_InputValues->SelectedFeatureDataArrayPaths[i];
    auto createdPath = m_InputValues->CreatedDataArrayPaths[i + numSelectedCellArrayPaths];
    ::AddFeatureTupleTransferInstance(m_DataStructure, selectedPath, createdPath, m_InputValues->FeatureIdsArrayPath, tupleTransferFunctions);
  }

  usize faceIndex = 0;
  //   Create temporary storage for cell quads which are constructed around edges
  //   crossed by the surface. Handle 3 edges per cell. The other 9 cell edges will
  //   be handled when neighboring cells that share edges with this cell are visited.
  std::array<usize, 3> t1 = {0, 0, 0};
  std::array<usize, 3> t2 = {0, 0, 0};
  std::array<size_t, 6> triangleVtxIDs = {0, 0, 0, 0, 0, 0};
  std::array<size_t, 4> vertexIndices = {0, 0, 0, 0};
  std::array<LabelType, 2> quadLabels = {0, 0};
  std::array<VertexData, 4> vData{};
  std::array<int32_t, 3> cellIndex = {0, 0, 0};

  for(int idxVtx = 0; idxVtx < nodeCount; idxVtx++)
  {
    cellMapPtr->getVertexCellIndex(idxVtx, cellIndex.data());
    // Back-bottom edge
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()))
    {
      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);
      if(quadLabels[0] == MMSurfaceNet::Padding)
      {
        quadLabels[0] = 0;
      }
      if(quadLabels[1] == MMSurfaceNet::Padding)
      {
        quadLabels[1] = 0;
      }

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
      for(const auto& tupleTransferFunction : tupleTransferFunctions)
      {
        tupleTransferFunction->surfaceNetsTransfer(faceIndex, quadNxArrayIndices);
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
      for(const auto& tupleTransferFunction : tupleTransferFunctions)
      {
        tupleTransferFunction->surfaceNetsTransfer(faceIndex, quadNxArrayIndices);
      }
      faceIndex++;
    }

    // Left-bottom edge
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()))
    {
      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]); ///
      if(quadLabels[0] == MMSurfaceNet::Padding)
      {
        quadLabels[0] = 0;
      }
      if(quadLabels[1] == MMSurfaceNet::Padding)
      {
        quadLabels[1] = 0;
      }
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
      for(const auto& tupleTransferFunction : tupleTransferFunctions)
      {
        tupleTransferFunction->surfaceNetsTransfer(faceIndex, quadNxArrayIndices);
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
      for(const auto& tupleTransferFunction : tupleTransferFunctions)
      {
        tupleTransferFunction->surfaceNetsTransfer(faceIndex, quadNxArrayIndices);
      }
      faceIndex++;
    }

    // Left-back edge
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()))
    {
      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);
      if(quadLabels[0] == MMSurfaceNet::Padding)
      {
        quadLabels[0] = 0;
      }
      if(quadLabels[1] == MMSurfaceNet::Padding)
      {
        quadLabels[1] = 0;
      }
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
      for(const auto& tupleTransferFunction : tupleTransferFunctions)
      {
        tupleTransferFunction->surfaceNetsTransfer(faceIndex, quadNxArrayIndices);
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
      for(const auto& tupleTransferFunction : tupleTransferFunctions)
      {
        tupleTransferFunction->surfaceNetsTransfer(faceIndex, quadNxArrayIndices);
      }
      faceIndex++;
    }
  }

  // Now run through the FaceLabels to make them consistent with Quick Surface Mesh
  for(usize tIdx = 0; tIdx < triangleCount * 2; tIdx++)
  {
    if(faceLabels[tIdx] == 0)
    {
      faceLabels[tIdx] = -1;
    }
  }

  // Scoped because we invalidate connectivity at the end
  Result<> windingResult = {};
  if(m_InputValues->RepairTriangleWinding)
  {
    // Generate Connectivity
    m_MessageHandler("Generating Connectivity and Triangle Neighbors...");
    triangleGeom.findElementNeighbors(true);
    const auto optionalId = triangleGeom.getElementNeighborsId();
    if(!optionalId.has_value())
    {
      return MakeErrorResult(-56331, fmt::format("Unable to generate the connectivity list for {} geometry.", triangleGeom.getName()));
    }
    const auto& connectivity = m_DataStructure.getDataRefAs<AbstractGeometry::ElementDynamicList>(optionalId.value());

    m_MessageHandler("Repairing Windings...");

    windingResult = MeshingUtilities::RepairTriangleWinding(triangleGeom.getFaces()->getDataStoreRef(), connectivity,
                                                            m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef(), m_ShouldCancel, m_MessageHandler);

    // Purge connectivity
    m_DataStructure.removeData(triangleGeom.getElementContainingVertId().value());
    m_DataStructure.removeData(triangleGeom.getElementNeighborsId().value());
  }

  return windingResult;
}
