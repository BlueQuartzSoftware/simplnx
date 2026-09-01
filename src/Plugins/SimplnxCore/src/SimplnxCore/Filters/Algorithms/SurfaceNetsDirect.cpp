/**
 * @file SurfaceNetsDirect.cpp
 * @brief Implements resident Surface Nets mesh generation.
 */

#include "SurfaceNetsDirect.hpp"

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

#include <fmt/format.h>

#include <limits>

using namespace nx::core;

namespace
{
using LabelType = int32;

/** @brief Returns true when the selected mode omits a padding-to-background quad. */
bool SkipPaddingQuad(ChoicesParameter::ValueType mode, const std::array<LabelType, 2>& quadLabels)
{
  if(mode != BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
  {
    return false;
  }
  return (quadLabels[0] == MMSurfaceNet::Padding && quadLabels[1] == 0) || (quadLabels[1] == MMSurfaceNet::Padding && quadLabels[0] == 0);
}

/**
 * @struct VertexData
 * @brief Stores a mesh vertex ID and position for quad triangulation.
 */
struct VertexData
{
  usize VertexId = 0;
  std::array<float32, 3> Position;
};

/**
 * @brief Computes a three-dimensional cross product.
 * @param vert0 First vector.
 * @param vert1 Second vector.
 * @param result Local output copy. The caller's array is unchanged.
 */
void crossProduct(const std::array<float32, 3>& vert0, const std::array<float32, 3> vert1, std::array<float32, 3> result)
{
  result[0] = vert0[1] * vert1[2] - vert0[2] * vert1[1];
  result[1] = vert0[2] * vert1[0] - vert0[0] * vert1[2];
  result[2] = vert0[0] * vert1[1] - vert0[1] * vert1[0];
}
/**
 * @brief Computes a triangle area from three positions.
 * @param vert0 First position.
 * @param vert1 Second position.
 * @param vert2 Third position.
 * @return Zero because crossProduct() receives its result by value.
 */
float32 triangleArea(std::array<float32, 3>& vert0, std::array<float32, 3>& vert1, std::array<float32, 3>& vert2)
{
  const std::array<float32, 3> v01 = {vert1[0] - vert0[0], vert1[1] - vert0[1], vert1[2] - vert0[2]};
  const std::array<float32, 3> v02 = {vert2[0] - vert0[0], vert2[1] - vert0[1], vert2[2] - vert0[2]};
  std::array<float32, 3> cross = {0.0f, 0.0f, 0.0f};
  crossProduct(v01, v02, cross);
  float32 const magCP = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
  return 0.5f * magCP;
}

/**
 * @brief Orients a quad and selects its lower-area triangulation.
 * @param vData Quad vertices, reordered in place.
 * @param isQuadFrontFacing True when the initial order faces forward.
 * @param triangleVtxIDs Receives two three-vertex triangles.
 *
 * Both area results are zero because crossProduct() cannot update its caller.
 * Current call sites also supply zero positions. The first diagonal remains.
 */
void getQuadTriangleIDs(std::array<VertexData, 4>& vData, bool isQuadFrontFacing, std::array<usize, 6>& triangleVtxIDs)
{
  // Swap the side vertices when label order indicates back-facing winding.
  if(!isQuadFrontFacing)
  {
    VertexData const temp = vData[3];
    vData[3] = vData[1];
    vData[1] = temp;
  }

  // Prefer the lower-area diagonal when positions distinguish the alternatives.
  float32 const thisArea = triangleArea(vData[0].Position, vData[1].Position, vData[2].Position) + triangleArea(vData[0].Position, vData[2].Position, vData[3].Position);
  float32 const alternateArea = triangleArea(vData[1].Position, vData[2].Position, vData[3].Position) + triangleArea(vData[1].Position, vData[3].Position, vData[0].Position);
  if(alternateArea < thisArea)
  {
    VertexData const temp = vData[0];
    vData[0] = vData[1];
    vData[1] = vData[2];
    vData[2] = vData[3];
    vData[3] = temp;
  }

  // Emit two triangles as a fan from vertex zero.
  triangleVtxIDs[0] = vData[0].VertexId;
  triangleVtxIDs[1] = vData[1].VertexId;
  triangleVtxIDs[2] = vData[2].VertexId;
  triangleVtxIDs[3] = vData[0].VertexId;
  triangleVtxIDs[4] = vData[2].VertexId;
  triangleVtxIDs[5] = vData[3].VertexId;
}
} // namespace

SurfaceNetsDirect::SurfaceNetsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const SurfaceNetsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

SurfaceNetsDirect::~SurfaceNetsDirect() noexcept = default;

Result<> SurfaceNetsDirect::operator()()
{
  const auto& featureIdsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  Result<> sentinelCheck = MeshingUtilities::ValidateFeatureIdsAgainstSentinels(featureIdsStore, m_InputValues->FeatureIdsArrayPath, true, m_ShouldCancel, m_MessageHandler);
  if(sentinelCheck.invalid())
  {
    return sentinelCheck;
  }

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GridGeomDataPath);

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  auto* triangleGeomPtr = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  auto gridDimensions = imageGeom.getDimensions();
  auto voxelSize = imageGeom.getSpacing();
  auto origin = imageGeom.getOrigin();

  // MMSurfaceNet classifies the complete padded grid through direct Feature ID reads.
  MMSurfaceNet surfaceNet(triangleGeomPtr->getVerticesRef().getDataStoreRef(), m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath), gridDimensions.data(), voxelSize.data());
  if(!surfaceNet.getCellMap()->valid())
  {
    return MakeErrorResult(-843870, fmt::format("Could not allocate SurfaceNets internal data structures for grid geometry at path '{}'.", m_InputValues->GridGeomDataPath.toString()));
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // Optional relaxation uses face neighbors and clamps local cell coordinates.
  if(m_InputValues->ApplySmoothing)
  {
    MMSurfaceNet::RelaxAttrs relaxAttrs{};
    relaxAttrs.maxDistFromCellCenter = m_InputValues->MaxDistanceFromVoxel;
    relaxAttrs.numRelaxIterations = m_InputValues->SmoothingIterations;
    relaxAttrs.relaxFactor = m_InputValues->RelaxationFactor;

    surfaceNet.relax(relaxAttrs);
  }

  auto cellMapPtr = surfaceNet.getCellMap();
  const usize nodeCount = cellMapPtr->numVertices();

  std::array<int, 3> arraySize2 = {0, 0, 0};
  cellMapPtr->getArraySize(arraySize2.data());

  triangleGeom.getVertexAttributeMatrix()->resizeTuples({static_cast<usize>(nodeCount)});

  auto& nodeTypes = m_DataStructure.getDataAs<Int8Array>(m_InputValues->NodeTypesDataPath)->getDataStoreRef();
  nodeTypes.resizeTuples({static_cast<usize>(nodeCount)});

  // Transform local cell positions and assign junction-count node types.
  Point3Df position = {0.0f, 0.0f, 0.0f};

  std::array<int, 3> vertCellIndex = {0, 0, 0};
  for(usize vertIndex = 0; vertIndex < nodeCount; vertIndex++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    cellMapPtr->getVertexPosition(vertIndex, position.data());
    // Relocate the vertex correctly based on the origin of the ImageGeometry
    position = position + origin - Point3Df(0.5f * voxelSize[0], 0.5f * voxelSize[1], 0.5f * voxelSize[2]);

    triangleGeom.setVertexCoordinate(static_cast<usize>(vertIndex), position);
    cellMapPtr->getVertexCellIndex(vertIndex, vertCellIndex.data());
    nodeTypes[static_cast<usize>(vertIndex)] = cellMapPtr->nodeType(vertCellIndex.data());
  }

  // Three owned edges per surface vertex count each crossing once.
  usize triangleCount = 0;
  usize suppressedFaceCount = 0;
  std::array<usize, 2> quadNxArrayIndices = {0, 0};
  // Count before allocation because each crossing produces two faces.
  for(int idxVtx = 0; idxVtx < nodeCount; idxVtx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    std::array<usize, 4> vertexIndices = {0, 0, 0, 0};
    std::array<::LabelType, 2> quadLabels = {0, 0};

    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()))
    {
      if(SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
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
      if(SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
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
      if(SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
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

  auto& faceLabels = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef();
  faceLabels.resizeTuples({triangleCount});

  // Match each selected cell array with its created two-sided face array.
  std::vector<std::shared_ptr<AbstractTupleTransfer>> tupleTransferFunctions;
  for(usize i = 0; i < m_InputValues->SelectedCellDataArrayPaths.size(); i++)
  {
    ::AddTupleTransferInstance(m_DataStructure, m_InputValues->SelectedCellDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i], tupleTransferFunctions);
  }

  auto numSelectedCellArrayPaths = m_InputValues->SelectedCellDataArrayPaths.size();

  for(usize i = 0; i < m_InputValues->SelectedFeatureDataArrayPaths.size(); i++)
  {
    auto selectedPath = m_InputValues->SelectedFeatureDataArrayPaths[i];
    auto createdPath = m_InputValues->CreatedDataArrayPaths[i + numSelectedCellArrayPaths];
    ::AddFeatureTupleTransferInstance(m_DataStructure, selectedPath, createdPath, m_InputValues->FeatureIdsArrayPath, tupleTransferFunctions);
  }

  // Generate connectivity, labels, and tuple transfers in the count-pass order.
  usize faceIndex = 0;
  // Other cell edges belong to neighboring surface vertices.
  std::array<usize, 3> t1 = {0, 0, 0};
  std::array<usize, 3> t2 = {0, 0, 0};
  std::array<usize, 6> triangleVtxIDs = {0, 0, 0, 0, 0, 0};
  std::array<usize, 4> vertexIndices = {0, 0, 0, 0};
  std::array<LabelType, 2> quadLabels = {0, 0};
  std::array<VertexData, 4> vData{};
  std::array<int32, 3> cellIndex = {0, 0, 0};

  for(int idxVtx = 0; idxVtx < nodeCount; idxVtx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    cellMapPtr->getVertexCellIndex(idxVtx, cellIndex.data());
    // Back-bottom edge
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()) &&
       !SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
    {
      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);
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
      // Direct tuple transfer uses per-value source and destination access.
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
      for(const auto& tupleTransferFunction : tupleTransferFunctions)
      {
        tupleTransferFunction->surfaceNetsTransfer(faceIndex, quadNxArrayIndices);
      }
      faceIndex++;
    }

    // Left-bottom edge
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()) &&
       !SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
    {
      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);
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
      for(const auto& tupleTransferFunction : tupleTransferFunctions)
      {
        tupleTransferFunction->surfaceNetsTransfer(faceIndex, quadNxArrayIndices);
      }
      faceIndex++;
    }

    // Left-back edge
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices.data(), quadLabels.data(), quadNxArrayIndices.data()) &&
       !SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
    {
      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);
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
      for(const auto& tupleTransferFunction : tupleTransferFunctions)
      {
        tupleTransferFunction->surfaceNetsTransfer(faceIndex, quadNxArrayIndices);
      }
      faceIndex++;
    }
  }

  if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
  {
    auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();
    const usize numFaceIndices = triangleCount * 3;
    std::vector<bool> isReferenced(nodeCount, false);
    for(usize i = 0; i < numFaceIndices; i++)
    {
      isReferenced[static_cast<usize>(facesRef[i])] = true;
    }

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
      for(usize i = 0; i < numFaceIndices; i++)
      {
        facesRef[i] = static_cast<IGeometry::MeshIndexType>(newVertexIndex[static_cast<usize>(facesRef[i])]);
      }
      triangleGeom.resizeVertexList(survivingVertexCount);
      triangleGeom.getVertexAttributeMatrix()->resizeTuples({survivingVertexCount});
      nodeTypes.resizeTuples({survivingVertexCount});
    }
  }

  // Temporary connectivity exists only for optional resident winding repair.
  Result<> windingResult = {};
  if(m_InputValues->RepairTriangleWinding)
  {
    m_MessageHandler("Generating Connectivity and Triangle Neighbors...");
    triangleGeom.findElementNeighbors(true);
    const auto optionalId = triangleGeom.getElementNeighborsId();
    if(!optionalId.has_value())
    {
      return MakeErrorResult(-56331, fmt::format("Unable to generate the connectivity list for {} geometry.", triangleGeom.getName()));
    }
    const auto& connectivity = m_DataStructure.getDataRefAs<IGeometry::ElementDynamicList>(optionalId.value());

    m_MessageHandler("Repairing Windings...");

    windingResult = MeshingUtilities::RepairTriangleWinding(triangleGeom.getFaces()->getDataStoreRef(), connectivity,
                                                            m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef(), m_ShouldCancel, m_MessageHandler);

    // Remove temporary connectivity after winding repair.
    m_DataStructure.removeData(triangleGeom.getElementContainingVertId().value());
    m_DataStructure.removeData(triangleGeom.getElementNeighborsId().value());
  }

  if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && windingResult.valid())
  {
    if(triangleGeom.getNumberOfFaces() == 0)
    {
      return MeshingUtilities::MakeEmptyMeshWarning(m_InputValues->TriangleGeometryPath, m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getNumberOfTuples(),
                                                    triangleGeom.getNumberOfVertices());
    }
    if(suppressedFaceCount == 0)
    {
      return MeshingUtilities::MakeNoFacesPrunedWarning(m_InputValues->TriangleGeometryPath);
    }
  }

  return windingResult;
}
