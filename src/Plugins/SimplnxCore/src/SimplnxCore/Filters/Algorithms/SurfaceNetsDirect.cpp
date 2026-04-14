/**
 * @file SurfaceNetsDirect.cpp
 * @brief In-core implementation of the SurfaceNets algorithm using the MMSurfaceNet library.
 *
 * This file delegates cell classification, vertex placement, and optional
 * smoothing to the MMSurfaceNet library, which operates on the full padded
 * grid in memory. The algorithm then extracts quads from edge crossings,
 * triangulates them, and writes the output TriangleGeom.
 *
 * The MMSurfaceNet library accesses the FeatureIds array via operator[], which
 * is efficient for in-memory DataStores but would cause severe chunk thrashing
 * on OOC stores. This is why SurfaceNetsScanline exists as an alternative.
 *
 * ## Phases
 *
 * Phase 1: MMSurfaceNet constructs a padded grid (dimX+2, dimY+2, dimZ+2) and
 *          classifies each cell by examining its 8 corner labels. Surface cells
 *          (where not all corners match) get vertices at cell centers.
 *
 * Phase 2: Optional smoothing via MMSurfaceNet::relax() moves vertices toward
 *          neighbor averages, clamped to MaxDistanceFromVoxel.
 *
 * Phase 3: Vertex positions are transformed from local cell coordinates to
 *          world coordinates using ImageGeom origin and spacing.
 *
 * Phase 4: First pass counts triangles by checking 3 edges per surface vertex.
 *
 * Phase 5: Second pass generates triangle connectivity and face labels.
 *          Each edge crossing produces a quad (4 vertices), which is split
 *          into 2 triangles using the minimal-area diagonal.
 *
 * Phase 6: Optional winding repair.
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
#include "simplnx/Utilities/MessageHelper.hpp"

#include "SimplnxCore/SurfaceNets/MMCellFlag.h"
#include "SimplnxCore/SurfaceNets/MMCellMap.h"
#include "SimplnxCore/SurfaceNets/MMGeometryOBJ.h"
#include "SimplnxCore/SurfaceNets/MMSurfaceNet.h"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
using LabelType = int32;
/**
 * @brief Adjusts a node type value for vertices on the exterior boundary.
 * Values < 10 get +10 added to indicate they are boundary nodes.
 * Values >= 10 are already marked as boundary and get +1.
 */
constexpr inline int8 CalculatePadding(int8 value)
{
  return value + ((9 * static_cast<int8>(value < 10)) + 1);
}

/**
 * @brief Marks all 4 vertices of a quad as boundary nodes when one of the
 * quad's labels is MMSurfaceNet::Padding (i.e., the exterior of the volume).
 */
inline void HandlePadding(std::array<usize, 4> vertexIndices, AbstractDataStore<int8>& nodeTypes)
{
  nodeTypes.setValue(vertexIndices[0], CalculatePadding(nodeTypes.getValue(vertexIndices[0])));
  nodeTypes.setValue(vertexIndices[1], CalculatePadding(nodeTypes.getValue(vertexIndices[1])));
  nodeTypes.setValue(vertexIndices[2], CalculatePadding(nodeTypes.getValue(vertexIndices[2])));
  nodeTypes.setValue(vertexIndices[3], CalculatePadding(nodeTypes.getValue(vertexIndices[3])));
};

struct VertexData
{
  usize VertexId = 0;
  std::array<float32, 3> Position;
};

void crossProduct(const std::array<float32, 3>& vert0, const std::array<float32, 3> vert1, std::array<float32, 3> result)
{
  // Cross product of vectors v0 and v1
  result[0] = vert0[1] * vert1[2] - vert0[2] * vert1[1];
  result[1] = vert0[2] * vert1[0] - vert0[0] * vert1[2];
  result[2] = vert0[0] * vert1[1] - vert0[1] * vert1[0];
}
float32 triangleArea(std::array<float32, 3>& vert0, std::array<float32, 3>& vert1, std::array<float32, 3>& vert2)
{
  // Area of triangle with vertex positions p0, p1, p2
  const std::array<float32, 3> v01 = {vert1[0] - vert0[0], vert1[1] - vert0[1], vert1[2] - vert0[2]};
  const std::array<float32, 3> v02 = {vert2[0] - vert0[0], vert2[1] - vert0[1], vert2[2] - vert0[2]};
  std::array<float32, 3> cross = {0.0f, 0.0f, 0.0f};
  crossProduct(v01, v02, cross);
  float32 const magCP = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
  return 0.5f * magCP;
}

/**
 * @brief Splits a quad into 2 triangles with consistent winding and minimal area.
 *
 * The quad is defined by 4 vertices in vData. The function:
 *   1. Flips winding if the quad is back-facing (swaps vertices 1 and 3)
 *   2. Chooses the triangulation diagonal that minimizes total triangle area,
 *      which reduces self-intersections in the resulting mesh
 *   3. Writes 6 vertex IDs into triangleVtxIDs (2 triangles x 3 vertices)
 *
 * @param[in,out] vData The 4 quad vertices (may be reordered for winding/area)
 * @param isQuadFrontFacing Whether labels[0] < labels[1] (determines initial winding)
 * @param[out] triangleVtxIDs 6 vertex IDs forming 2 triangles
 */
void getQuadTriangleIDs(std::array<VertexData, 4>& vData, bool isQuadFrontFacing, std::array<usize, 6>& triangleVtxIDs)
{
  // Step 1: Ensure consistent front-facing winding by swapping vertices 1 and 3
  if(!isQuadFrontFacing)
  {
    VertexData const temp = vData[3];
    vData[3] = vData[1];
    vData[1] = temp;
  }

  // Step 2: Choose the triangulation diagonal (0-2 vs 1-3) that minimizes
  // total triangle area, reducing self-intersections in the surface mesh
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

  // Step 3: Output the 2 triangles from the quad (fan triangulation from vData[0])
  triangleVtxIDs[0] = vData[0].VertexId;
  triangleVtxIDs[1] = vData[1].VertexId;
  triangleVtxIDs[2] = vData[2].VertexId;
  triangleVtxIDs[3] = vData[0].VertexId;
  triangleVtxIDs[4] = vData[2].VertexId;
  triangleVtxIDs[5] = vData[3].VertexId;
}
} // namespace

// -----------------------------------------------------------------------------
SurfaceNetsDirect::SurfaceNetsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const SurfaceNetsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SurfaceNetsDirect::~SurfaceNetsDirect() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Executes the full in-core Surface Nets pipeline.
 *
 * Delegates cell classification and smoothing to the MMSurfaceNet library,
 * then extracts edge-crossing quads and triangulates them. The MMSurfaceNet
 * constructor reads the entire FeatureIds array via operator[], which is
 * efficient for in-memory stores but would thrash on OOC stores.
 */
Result<> SurfaceNetsDirect::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GridGeomDataPath);

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  auto* triangleGeomPtr = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  auto gridDimensions = imageGeom.getDimensions();
  auto voxelSize = imageGeom.getSpacing();
  auto origin = imageGeom.getOrigin();

  // Phase 1: Build the surface net. MMSurfaceNet classifies every cell in a
  // padded grid (dim+2 in each direction) by examining the 8 corner labels.
  // Cells where not all corners match get a vertex at the cell center.
  // This reads the entire FeatureIds array via operator[] -- fast in-core only.
  messageHelper.sendMessage("Phase 1: Building surface net...");
  MMSurfaceNet surfaceNet(triangleGeomPtr->getVerticesRef().getDataStoreRef(), m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath), gridDimensions.data(), voxelSize.data());
  if(!surfaceNet.getCellMap()->valid())
  {
    return MakeErrorResult(-843870, fmt::format("Could not allocate SurfaceNets internal data structures for grid geometry at path '{}'.", m_InputValues->GridGeomDataPath.toString()));
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // Phase 2: Optional smoothing -- iterative Laplacian-like relaxation that
  // moves each vertex toward the average of its face-connected neighbors,
  // clamped to stay within MaxDistanceFromVoxel of the cell center.
  if(m_InputValues->ApplySmoothing)
  {
    messageHelper.sendMessage("Phase 2: Smoothing surface net...");
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

  // Remove and then insert a properly sized int8 for the NodeTypes
  auto& nodeTypes = m_DataStructure.getDataAs<Int8Array>(m_InputValues->NodeTypesDataPath)->getDataStoreRef();
  nodeTypes.resizeTuples({static_cast<usize>(nodeCount)});

  // Phase 3: Transform vertex positions from local cell-relative coordinates
  // (where 0.5 = cell center) to world coordinates using origin and spacing.
  // Also assigns node types from the MMCellFlag junction count.
  messageHelper.sendMessage("Phase 3: Transforming vertex positions...");
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
    position = position + origin - Point3Df(0.5f * voxelSize[0], 0.5f * voxelSize[1], 0.5f * voxelSize[1]);

    triangleGeom.setVertexCoordinate(static_cast<usize>(vertIndex), position);
    cellMapPtr->getVertexCellIndex(vertIndex, vertCellIndex.data());
    MMCellMap::Cell* currentCellPtr = cellMapPtr->getCell(vertCellIndex.data());
    nodeTypes[static_cast<usize>(vertIndex)] = static_cast<int8>(currentCellPtr->flag.numJunctions());
  }

  // Phase 4: Count triangles by checking 3 edges per surface vertex.
  // Each cell has 12 edges, but by convention only 3 are checked per vertex
  // (BackBottom, LeftBottom, LeftBack), which ensures each edge is counted
  // exactly once across the grid. Each edge crossing produces a quad = 2 triangles.
  messageHelper.sendMessage("Phase 4: Counting triangles...");
  usize triangleCount = 0;
  std::array<usize, 2> quadNxArrayIndices = {0, 0};
  // First pass: count only, do not write any mesh data
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
  for(usize i = 0; i < m_InputValues->SelectedCellDataArrayPaths.size(); i++)
  {
    // Associate these arrays with the Triangle Face Data.
    ::AddTupleTransferInstance(m_DataStructure, m_InputValues->SelectedCellDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i], tupleTransferFunctions);
  }

  auto numSelectedCellArrayPaths = m_InputValues->SelectedCellDataArrayPaths.size();

  for(usize i = 0; i < m_InputValues->SelectedFeatureDataArrayPaths.size(); i++)
  {
    // Associate these arrays with the Triangle Face Data.
    auto selectedPath = m_InputValues->SelectedFeatureDataArrayPaths[i];
    auto createdPath = m_InputValues->CreatedDataArrayPaths[i + numSelectedCellArrayPaths];
    ::AddFeatureTupleTransferInstance(m_DataStructure, selectedPath, createdPath, m_InputValues->FeatureIdsArrayPath, tupleTransferFunctions);
  }

  // Phase 5: Generate triangles. Same edge iteration as Phase 4, but now
  // writes triangle connectivity, face labels, and runs TupleTransfer.
  // Each edge crossing produces a quad defined by 4 vertices from the current
  // cell and 3 neighboring cells. The quad is split into 2 triangles using
  // the minimal-area diagonal via getQuadTriangleIDs().
  messageHelper.sendMessage("Phase 5: Generating triangles...");
  usize faceIndex = 0;
  // Handle 3 edges per cell (BackBottom, LeftBottom, LeftBack). The other 9
  // cell edges are handled when neighboring cells that share those edges are visited.
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

  // Replace Padding label (0) with -1 to match QuickSurfaceMesh convention
  // where -1 indicates the exterior of the volume
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
    messageHelper.sendMessage("Phase 6: Repairing triangle winding...");
    // Generate Connectivity
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

    // Purge connectivity
    m_DataStructure.removeData(triangleGeom.getElementContainingVertId().value());
    m_DataStructure.removeData(triangleGeom.getElementNeighborsId().value());
  }

  return windingResult;
}
