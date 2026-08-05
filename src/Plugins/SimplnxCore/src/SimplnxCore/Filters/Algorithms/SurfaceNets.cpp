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

#include <limits>
#include <vector>

using namespace nx::core;

namespace
{
using LabelType = int32;

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

/**
 * @brief True when this quad is bounding-box wall backed by background and must be skipped.
 * Called from both the counting pass and the emit pass -- they must agree exactly.
 * Note this reads the RAW quadLabels, where MMSurfaceNet::Padding is still distinct from a
 * real Feature Id 0. Do not call it after the Padding -> -1 remap.
 */
inline bool SkipPaddingQuad(ChoicesParameter::ValueType mode, const std::array<LabelType, 2>& quadLabels)
{
  if(mode != BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
  {
    return false;
  }
  return (quadLabels[0] == MMSurfaceNet::Padding && quadLabels[1] == 0) || (quadLabels[1] == MMSurfaceNet::Padding && quadLabels[0] == 0);
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
  // Reject Feature Ids that collide with MMSurfaceNet::Padding (INT32_MAX), the sentinel this
  // algorithm uses for "outside the volume" throughout MMCellMap: a real Feature Id of INT32_MAX
  // would be silently treated as exterior. This is a mitigation for the underlying
  // sentinel-collision design, not a fix -- see simplnx#1705. Run here (execute), not preflight: a
  // full-volume scan is too expensive to repeat on every GUI parameter edit.
  {
    const auto& featureIdsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
    Result<> sentinelCheck = MeshingUtilities::ValidateFeatureIdsAgainstSentinels(featureIdsStore, m_InputValues->FeatureIdsArrayPath, /*rejectMaxInt32=*/true, m_ShouldCancel, m_MessageHandler);
    if(sentinelCheck.invalid())
    {
      return sentinelCheck;
    }
  }

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
    return MakeErrorResult(-843870, fmt::format("Could not allocate SurfaceNets internal data structures for grid geometry at path '{}'.", m_InputValues->GridGeomDataPath.toString()));
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
    position = position + origin - Point3Df(0.5f * voxelSize[0], 0.5f * voxelSize[1], 0.5f * voxelSize[2]);

    triangleGeom.setVertexCoordinate(static_cast<usize>(vertIndex), position);
    cellMapPtr->getVertexCellIndex(vertIndex, vertCellIndex.data());
    nodeTypes[static_cast<usize>(vertIndex)] = cellMapPtr->nodeType(vertCellIndex.data());
  }

  usize triangleCount = 0;
  // Counts quads suppressed by the Bounding Box Skin option's 'Background-Backed Walls Only' mode
  // (BoundingBoxSkinMode::k_BackgroundBackedWallsOnly) in this counting pass. Always 0 when the mode is
  // Off. Lets the caller warn when the option is on but pruned nothing.
  usize suppressedFaceCount = 0;
  std::array<usize, 2> quadNxArrayIndices = {0, 0};
  // First Pass through to just count the number of triangles:
  for(int idxVtx = 0; idxVtx < nodeCount; idxVtx++)
  {
    std::array<size_t, 4> vertexIndices = {0, 0, 0, 0};
    std::array<::LabelType, 2> quadLabels = {0, 0};

    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()))
    {
      if(::SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
      {
        suppressedFaceCount++;
      }
      else
      {
        triangleCount += 2;
      }
    }
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()))
    {
      if(::SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
      {
        suppressedFaceCount++;
      }
      else
      {
        triangleCount += 2;
      }
    }
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()))
    {
      if(::SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
      {
        suppressedFaceCount++;
      }
      else
      {
        triangleCount += 2;
      }
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
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()) &&
       !::SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
    {
      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);
      // Map the exterior padding sentinel straight to the shared exterior Face Label (-1).
      // Going through 0 as an intermediate would collide with real Feature Id 0.
      if(quadLabels[0] == MMSurfaceNet::Padding)
      {
        quadLabels[0] = -1;
      }
      if(quadLabels[1] == MMSurfaceNet::Padding)
      {
        quadLabels[1] = -1;
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
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()) &&
       !::SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
    {
      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]); ///
      // Map the exterior padding sentinel straight to the shared exterior Face Label (-1).
      // Going through 0 as an intermediate would collide with real Feature Id 0.
      if(quadLabels[0] == MMSurfaceNet::Padding)
      {
        quadLabels[0] = -1;
      }
      if(quadLabels[1] == MMSurfaceNet::Padding)
      {
        quadLabels[1] = -1;
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
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()) &&
       !::SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
    {
      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);
      // Map the exterior padding sentinel straight to the shared exterior Face Label (-1).
      // Going through 0 as an intermediate would collide with real Feature Id 0.
      if(quadLabels[0] == MMSurfaceNet::Padding)
      {
        quadLabels[0] = -1;
      }
      if(quadLabels[1] == MMSurfaceNet::Padding)
      {
        quadLabels[1] = -1;
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

  // Dropping faces can orphan vertices, which come from the cell map before any triangle
  // exists. Compact them so the vertex list, Node Types and vertex AttributeMatrix agree.
  // Skipped entirely when the option is off, because then no face was dropped.
  if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
  {
    m_MessageHandler.sendInfoMessage("Removing vertices orphaned by omitted bounding box faces...");

    auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();
    const usize numFaceIndices = triangleCount * 3;

    // First pass: find which of the nodeCount vertices are actually referenced by a face.
    std::vector<bool> isReferenced(nodeCount, false);
    for(usize i = 0; i < numFaceIndices; i++)
    {
      isReferenced[static_cast<usize>(facesRef[i])] = true;
    }

    // Second pass: assign each referenced vertex's new index and copy its data in the same
    // step, walking in ascending order of OLD index (not order of first use in the face
    // list). Assigning destIndex from a running counter as oldIndex ascends guarantees
    // newVertexIndex[oldIndex] <= oldIndex for every referenced vertex, which is what makes
    // the in-place copy below safe -- a destination slot is always at or before its source
    // slot, so writing it never clobbers data that a later iteration still needs to read.
    // The two passes this replaces (assign-then-copy) walked all nodeCount vertices twice for
    // no benefit, since the copy consumes newVertexIndex[oldIndex] immediately after it is
    // assigned.
    constexpr usize k_NotUsed = std::numeric_limits<usize>::max();
    std::vector<usize> newVertexIndex(nodeCount, k_NotUsed);
    usize survivingVertexCount = 0;
    auto& verticesRef = triangleGeom.getVertices()->getDataStoreRef();
    for(usize oldIndex = 0; oldIndex < nodeCount; oldIndex++)
    {
      if(!isReferenced[oldIndex])
      {
        continue;
      }
      const usize destIndex = survivingVertexCount;
      newVertexIndex[oldIndex] = destIndex;
      for(usize comp = 0; comp < 3; comp++)
      {
        verticesRef[destIndex * 3 + comp] = verticesRef[oldIndex * 3 + comp];
      }
      nodeTypes[destIndex] = nodeTypes[oldIndex];
      survivingVertexCount++;
    }

    if(survivingVertexCount < nodeCount)
    {
      // Remap the face indices to the compacted vertex ids. Must run after the copy loop
      // above (it reads the pre-remap face indices to look up newVertexIndex).
      for(usize i = 0; i < numFaceIndices; i++)
      {
        facesRef[i] = static_cast<IGeometry::MeshIndexType>(newVertexIndex[static_cast<usize>(facesRef[i])]);
      }

      triangleGeom.resizeVertexList(survivingVertexCount);
      triangleGeom.getVertexAttributeMatrix()->resizeTuples({survivingVertexCount});
      nodeTypes.resizeTuples({survivingVertexCount});
    }
  }

  // Scoped because we invalidate connectivity at the end
  Result<> windingResult = {};
  if(m_InputValues->RepairTriangleWinding)
  {
    // Generate Connectivity
    m_MessageHandler.sendInfoMessage("Generating Connectivity and Triangle Neighbors...");
    triangleGeom.findElementNeighbors(true);
    const auto optionalId = triangleGeom.getElementNeighborsId();
    if(!optionalId.has_value())
    {
      return MakeErrorResult(-56331, fmt::format("Unable to generate the connectivity list for {} geometry.", triangleGeom.getName()));
    }
    const auto& connectivity = m_DataStructure.getDataRefAs<IGeometry::ElementDynamicList>(optionalId.value());

    m_MessageHandler.sendInfoMessage("Repairing Windings...");

    windingResult = MeshingUtilities::RepairTriangleWinding(triangleGeom.getFaces()->getDataStoreRef(), connectivity,
                                                            m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef(), m_ShouldCancel, m_MessageHandler);

    // Purge connectivity
    m_DataStructure.removeData(triangleGeom.getElementContainingVertId().value());
    m_DataStructure.removeData(triangleGeom.getElementNeighborsId().value());
  }

  // Guarded on windingResult still being valid so a genuine winding-repair error is never discarded.
  if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && windingResult.valid())
  {
    // An entirely-background volume has nothing but {-1, 0} faces, so omitting the skin
    // legitimately produces an empty mesh. Report it rather than returning silently.
    if(triangleGeom.getNumberOfFaces() == 0)
    {
      return MeshingUtilities::MakeEmptyMeshWarning(m_InputValues->TriangleGeometryPath, m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getNumberOfTuples(),
                                                    triangleGeom.getNumberOfVertices());
    }
    // A fully-indexed volume (no Feature Id 0) makes the option a no-op: nothing was pruned, and
    // the user otherwise gets byte-identical output with no feedback that the option had no effect.
    if(suppressedFaceCount == 0)
    {
      return MeshingUtilities::MakeNoFacesPrunedWarning(m_InputValues->TriangleGeometryPath);
    }
  }

  return windingResult;
}
