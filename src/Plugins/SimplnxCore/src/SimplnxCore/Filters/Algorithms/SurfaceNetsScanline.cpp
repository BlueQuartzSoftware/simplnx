/**
 * @file SurfaceNetsScanline.cpp
 * @brief Out-of-core (OOC) optimized implementation of the SurfaceNets algorithm.
 *
 * This file reimplements the Surface Nets algorithm without using the MMSurfaceNet
 * library (which requires O(volume) memory and per-element FeatureIds access).
 * Instead, it reads FeatureIds via copyIntoBuffer() in Z-slice pairs and stores
 * only the O(surface) cells that actually have vertices.
 *
 * ## Key Differences from SurfaceNetsDirect
 *
 * 1. **No MMSurfaceNet/MMCellMap**: The Direct variant delegates cell classification
 *    to MMSurfaceNet, which allocates a Cell for every padded voxel (O(volume)).
 *    The Scanline variant performs cell classification inline using MMCellFlag
 *    directly, storing only surface cells in a vector + hash map (O(surface)).
 *
 * 2. **FeatureIds access**: Direct passes the full array to MMSurfaceNet which
 *    reads via operator[]. Scanline reads two Z-slices at a time via
 *    copyIntoBuffer() and resolves cell corner labels from the buffered slices
 *    using the cornerLabel() helper.
 *
 * 3. **Smoothing**: Direct delegates to MMSurfaceNet::relax() which operates on
 *    the full MMCellMap. Scanline performs smoothing on the O(surface) m_Vertices
 *    array using neighbor lookups through the m_CellToVertex hash map.
 *
 * 4. **Edge quad generation**: Direct uses MMCellMap::getEdgeQuad() which looks
 *    up neighboring cells in the O(volume) cell array. Scanline uses
 *    lookupVertex() to find neighbors in the O(surface) hash map.
 *
 * 5. **Output writes**: Direct writes per-element. Scanline buffers all output
 *    (triangle connectivity, face labels, vertex positions, node types) and
 *    flushes via copyFromBuffer() in bulk.
 *
 * ## Memory Comparison
 *
 * For a 500x500x500 dataset with ~1% surface cells:
 *   - Direct (MMCellMap): ~500^3 * sizeof(Cell) = O(125M) entries
 *   - Scanline: ~500^3 * 0.01 = O(1.25M) entries + hash map overhead
 *
 * The Scanline variant uses roughly 100x less memory for cell classification.
 */

#include "SurfaceNetsScanline.hpp"

#include "SurfaceNets.hpp"
#include "TupleTransfer.hpp"

#include "SimplnxCore/SurfaceNets/MMCellFlag.h"
#include "SimplnxCore/SurfaceNets/MMSurfaceNet.h"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{
using LabelType = int32;

/**
 * @brief Packs a padded-grid (i,j,k) coordinate into a single uint64 key
 * for the cell-to-vertex hash map.
 */
inline uint64 packCellKey(int32 i, int32 j, int32 k, int32 paddedX, int32 paddedXY)
{
  return static_cast<uint64>(i) + static_cast<uint64>(j) * static_cast<uint64>(paddedX) + static_cast<uint64>(k) * static_cast<uint64>(paddedXY);
}

/**
 * @brief Adjusts a node type value for vertices on the exterior boundary.
 * Same logic as SurfaceNetsDirect but operates on the local buffer.
 */
constexpr inline int8 CalculatePadding(int8 value)
{
  return value + ((9 * static_cast<int8>(value < 10)) + 1);
}

/**
 * @brief Marks all 4 vertices of a quad as boundary nodes in the local buffer
 * when one of the quad's labels is MMSurfaceNet::Padding.
 */
inline void HandlePadding(std::array<usize, 4> vertexIndices, std::vector<int8>& nodeTypesBuf)
{
  nodeTypesBuf[vertexIndices[0]] = CalculatePadding(nodeTypesBuf[vertexIndices[0]]);
  nodeTypesBuf[vertexIndices[1]] = CalculatePadding(nodeTypesBuf[vertexIndices[1]]);
  nodeTypesBuf[vertexIndices[2]] = CalculatePadding(nodeTypesBuf[vertexIndices[2]]);
  nodeTypesBuf[vertexIndices[3]] = CalculatePadding(nodeTypesBuf[vertexIndices[3]]);
}

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

void getQuadTriangleIDs(std::array<VertexData, 4>& vData, bool isQuadFrontFacing, std::array<usize, 6>& triangleVtxIDs)
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

  // Generate vertex ids to triangulate the quad
  triangleVtxIDs[0] = vData[0].VertexId;
  triangleVtxIDs[1] = vData[1].VertexId;
  triangleVtxIDs[2] = vData[2].VertexId;
  triangleVtxIDs[3] = vData[0].VertexId;
  triangleVtxIDs[4] = vData[2].VertexId;
  triangleVtxIDs[5] = vData[3].VertexId;
}

/**
 * @brief Looks up the vertex index for a neighboring cell at padded (ci,cj,ck)
 * from the cell-to-vertex hash map. Returns max if not found (should not happen
 * for valid edge quads).
 */
inline usize lookupVertex(const std::unordered_map<uint64, usize>& cellToVertex, int32 ci, int32 cj, int32 ck, int32 paddedX, int32 paddedXY)
{
  const uint64 key = packCellKey(ci, cj, ck, paddedX, paddedXY);
  auto it = cellToVertex.find(key);
  if(it != cellToVertex.end())
  {
    return it->second;
  }
  return std::numeric_limits<usize>::max();
}

/**
 * @brief Computes the label for a cell at padded coordinates (ci,cj,ck)
 * by reading from the FeatureIds store. Boundary cells return Padding.
 */
inline int32 edgeCellLabel(int32 ci, int32 cj, int32 ck, int32 paddedX, int32 paddedY, int32 paddedZ, usize dimX, usize dimY, const AbstractDataStore<int32>& featureIdsStore)
{
  // Boundary padding check (same logic as MMCellMap::label)
  if(ci <= 0 || cj <= 0 || ck <= 0 || ci >= paddedX - 1 || cj >= paddedY - 1 || ck >= paddedZ - 1)
  {
    return MMSurfaceNet::ReservedLabel::Padding;
  }
  // Convert padded to NX coordinates
  const int32 nxX = ci - 1;
  const int32 nxY = cj - 1;
  const int32 nxZ = ck - 1;
  // Additional range check (matches the checks in MMCellMap::label)
  if(nxX < 0 || nxX >= static_cast<int32>(dimX) || nxY < 0 || nxY >= static_cast<int32>(dimY) || nxZ < 0)
  {
    return MMSurfaceNet::ReservedLabel::Padding;
  }
  const usize nxIdx = static_cast<usize>(nxZ) * dimY * dimX + static_cast<usize>(nxY) * dimX + static_cast<usize>(nxX);
  return featureIdsStore.getValue(nxIdx);
}

/**
 * @brief Computes the flat NX array index for a cell at padded (ci,cj,ck).
 * Returns max if outside the NX volume.
 */
inline usize edgeCellNxIndex(int32 ci, int32 cj, int32 ck, int32 paddedX, int32 paddedY, int32 paddedZ, usize dimX, usize dimY, usize dimZ)
{
  const int32 nxX = ci - 1;
  const int32 nxY = cj - 1;
  const int32 nxZ = ck - 1;
  if(nxX < 0 || nxX >= static_cast<int32>(dimX) || nxY < 0 || nxY >= static_cast<int32>(dimY) || nxZ < 0 || nxZ >= static_cast<int32>(dimZ))
  {
    return std::numeric_limits<usize>::max();
  }
  return static_cast<usize>(nxZ) * dimY * dimX + static_cast<usize>(nxY) * dimX + static_cast<usize>(nxX);
}

/**
 * @brief Returns the FeatureId label for a corner at padded coordinates (ci,cj,ck).
 *
 * Boundary corners (any coordinate at 0 or >= paddedDim-1) return MMSurfaceNet::Padding.
 * Interior corners look up the label from the appropriate slice buffer.
 *
 * @param ci Padded X coordinate of the corner
 * @param cj Padded Y coordinate of the corner
 * @param ck Padded Z coordinate of the corner
 * @param paddedX Number of padded cells in X (dimX + 2)
 * @param paddedY Number of padded cells in Y (dimY + 2)
 * @param paddedZ Number of padded cells in Z (dimZ + 2)
 * @param dimX Original NX dimension in X
 * @param dimY Original NX dimension in Y
 * @param slice0 Buffer holding NX Z-slice at index (currentK - 1)
 * @param slice0Z The NX Z-index that slice0 currently holds
 * @param slice1 Buffer holding NX Z-slice at index currentK
 * @param slice1Z The NX Z-index that slice1 currently holds
 */
inline int32 cornerLabel(int32 ci, int32 cj, int32 ck, int32 paddedX, int32 paddedY, int32 paddedZ, usize dimX, usize dimY, const std::vector<int32>& slice0, int32 slice0Z,
                         const std::vector<int32>& slice1, int32 slice1Z)
{
  // Boundary padding check
  if(ci <= 0 || cj <= 0 || ck <= 0 || ci >= paddedX - 1 || cj >= paddedY - 1 || ck >= paddedZ - 1)
  {
    return MMSurfaceNet::ReservedLabel::Padding;
  }

  // Convert padded corner to NX coordinates
  const int32 nxX = ci - 1;
  const int32 nxY = cj - 1;
  const int32 nxZ = ck - 1;

  // Compute the flat index within a single XY slice
  const usize sliceOffset = static_cast<usize>(nxY) * dimX + static_cast<usize>(nxX);

  // Look up from the correct slice buffer
  if(nxZ == slice0Z)
  {
    return slice0[sliceOffset];
  }
  if(nxZ == slice1Z)
  {
    return slice1[sliceOffset];
  }

  // Should not reach here if slices are managed correctly
  return MMSurfaceNet::ReservedLabel::Padding;
}
} // namespace

// -----------------------------------------------------------------------------
SurfaceNetsScanline::SurfaceNetsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const SurfaceNetsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SurfaceNetsScanline::~SurfaceNetsScanline() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Executes the full OOC Surface Nets pipeline.
 *
 * This method is the most complex OOC algorithm in the codebase because it
 * reimplements the entire Surface Nets algorithm from scratch without the
 * MMSurfaceNet library, using only O(surface) memory and bulk I/O.
 *
 * The method is organized into numbered sections (1-13) that correspond to
 * the phases documented in the header. The sections are:
 *
 *   1-4.   Cell classification: iterate padded grid, load Z-slice pairs,
 *          classify cells using MMCellFlag, store surface cells
 *   5.     Resize vertex arrays
 *   6.     Phase 2A: Optional smoothing using O(surface) data structures
 *   7.     Phase 2B: Transform to world coordinates, assign node types
 *   8.     Phase 3A: Count triangles from edge crossings
 *   9.     Phase 3B: Resize face arrays
 *   10.    Phase 3C: Setup TupleTransfer
 *   11.    Phase 3D: Generate triangles and buffer output
 *   12.    Phase 3E: Fix face labels (0 -> -1)
 *   12b.   Flush all buffered output via copyFromBuffer()
 *   13.    Phase 3F: Optional winding repair
 */
Result<> SurfaceNetsScanline::operator()()
{
  // -------------------------------------------------------------------------
  // 1. Get ImageGeom dimensions and compute padded dims.
  // The padded grid adds 1 cell of padding on each side in each dimension,
  // matching the convention used by MMSurfaceNet. Padding cells always have
  // the MMSurfaceNet::Padding label, which creates boundary triangles at the
  // volume edges.
  // -------------------------------------------------------------------------
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GridGeomDataPath);
  auto gridDimensions = imageGeom.getDimensions();

  const usize dimX = gridDimensions[0];
  const usize dimY = gridDimensions[1];
  const usize dimZ = gridDimensions[2];

  const int32 paddedX = static_cast<int32>(dimX) + 2;
  const int32 paddedY = static_cast<int32>(dimY) + 2;
  const int32 paddedZ = static_cast<int32>(dimZ) + 2;
  const int32 paddedXY = paddedX * paddedY;

  // -------------------------------------------------------------------------
  // 2. Get the FeatureIds DataStore reference
  // -------------------------------------------------------------------------
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  // -------------------------------------------------------------------------
  // 3. Allocate rolling slice buffers (2 NX Z-slices).
  // Each buffer holds one full XY plane of FeatureIds. The ping-pong scheme
  // ensures at most 2 Z-slices are in memory at once. The slice0Z/slice1Z
  // variables track which NX Z-index each buffer currently holds, to avoid
  // re-reading a slice that is already buffered.
  // -------------------------------------------------------------------------
  const usize sliceSize = dimX * dimY;
  std::vector<int32> sliceBufA(sliceSize);
  std::vector<int32> sliceBufB(sliceSize);

  // Pointers for ping-pong: slice0 is the "lower" Z-slice, slice1 is "upper".
  // These are swapped at the end of each k iteration so the "upper" becomes
  // "lower" for the next iteration.
  std::vector<int32>* slice0 = &sliceBufA;
  std::vector<int32>* slice1 = &sliceBufB;
  int32 slice0Z = -1; // NX Z-index currently in slice0 (-1 = empty)
  int32 slice1Z = -1; // NX Z-index currently in slice1 (-1 = empty)

  // -------------------------------------------------------------------------
  // 4. Phase 1: Cell classification.
  // Iterate cells in the same order as MMCellMap::setCellVertices():
  //   k in [0, paddedZ-2), j in [0, paddedY-2), i in [0, paddedX-2)
  //
  // For each cell, compute the 8 corner labels using cornerLabel(), which
  // reads from the buffered Z-slices. The corner ordering matches MMCellMap:
  //   [0]=(i,j,k) [1]=(i+1,j,k) [2]=(i+1,j+1,k) [3]=(i,j+1,k)
  //   [4]=(i,j,k+1) [5]=(i+1,j,k+1) [6]=(i+1,j+1,k+1) [7]=(i,j+1,k+1)
  //
  // MMCellFlag::set() classifies the cell: if all 8 labels are identical,
  // it is an interior cell (no vertex). Otherwise it is a surface cell
  // and gets stored in m_Vertices with its padded coordinates and flag.
  // -------------------------------------------------------------------------
  const usize totalPaddedZSlices = static_cast<usize>(paddedZ - 1);
  for(int32 k = 0; k < paddedZ - 1; k++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    // The 8 corners of a cell at padded (i,j,k) span NX Z-indices:
    //   bottom face corners: ck = k     => nxZ = k - 1
    //   top face corners:    ck = k + 1 => nxZ = k
    // So we need NX slices at Z = (k-1) and Z = k.
    const int32 needZ0 = k - 1; // NX Z for bottom face corners
    const int32 needZ1 = k;     // NX Z for top face corners

    // Load the two required slices
    if(needZ0 >= 0 && needZ0 < static_cast<int32>(dimZ))
    {
      if(slice0Z != needZ0 && slice1Z != needZ0)
      {
        // Load needZ0 into slice0
        featureIdsStore.copyIntoBuffer(static_cast<usize>(needZ0) * sliceSize, nonstd::span<int32>(slice0->data(), sliceSize));
        slice0Z = needZ0;
      }
    }
    if(needZ1 >= 0 && needZ1 < static_cast<int32>(dimZ))
    {
      if(slice0Z != needZ1 && slice1Z != needZ1)
      {
        // Load needZ1 into slice1
        featureIdsStore.copyIntoBuffer(static_cast<usize>(needZ1) * sliceSize, nonstd::span<int32>(slice1->data(), sliceSize));
        slice1Z = needZ1;
      }
    }

    for(int32 j = 0; j < paddedY - 1; j++)
    {
      for(int32 i = 0; i < paddedX - 1; i++)
      {
        // Compute 8 corner labels for cell at padded (i,j,k)
        // Corner ordering matches MMCellMap::setCellVertices():
        //   [0] = (i,   j,   k  )   left-back-bottom
        //   [1] = (i+1, j,   k  )   right-back-bottom
        //   [2] = (i+1, j+1, k  )   right-front-bottom
        //   [3] = (i,   j+1, k  )   left-front-bottom
        //   [4] = (i,   j,   k+1)   left-back-top
        //   [5] = (i+1, j,   k+1)   right-back-top
        //   [6] = (i+1, j+1, k+1)   right-front-top
        //   [7] = (i,   j+1, k+1)   left-front-top
        int32 cellLabels[8];

        cellLabels[0] = cornerLabel(i, j, k, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[1] = cornerLabel(i + 1, j, k, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[2] = cornerLabel(i + 1, j + 1, k, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[3] = cornerLabel(i, j + 1, k, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[4] = cornerLabel(i, j, k + 1, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[5] = cornerLabel(i + 1, j, k + 1, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[6] = cornerLabel(i + 1, j + 1, k + 1, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[7] = cornerLabel(i, j + 1, k + 1, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);

        MMCellFlag flag;
        flag.clear();
        flag.set(cellLabels);

        if(flag.vertexType() != MMCellFlag::VertexType::NoVertex)
        {
          const usize vertexIndex = m_Vertices.size();
          m_Vertices.push_back(VertexInfo{{i, j, k}, flag});
          m_CellToVertex[packCellKey(i, j, k, paddedX, paddedXY)] = vertexIndex;
        }
      }
    }

    // Roll slice buffers for the next k iteration.
    // After processing k, the "bottom" slice (needZ0 = k-1) is no longer
    // needed for k+1 (which will need k and k+1). So we swap so that
    // the current "top" slice becomes the new "bottom" and the old
    // "bottom" buffer is available for loading.
    std::swap(slice0, slice1);
    std::swap(slice0Z, slice1Z);
  }

  // -------------------------------------------------------------------------
  // 5. Resize TriangleGeom vertex array and associated attribute arrays
  // -------------------------------------------------------------------------
  const usize numVertices = m_Vertices.size();

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  auto& verticesStore = triangleGeom.getVerticesRef().getDataStoreRef();
  verticesStore.resizeTuples(ShapeType{numVertices});

  triangleGeom.getVertexAttributeMatrix()->resizeTuples({numVertices});

  auto& nodeTypes = m_DataStructure.getDataAs<Int8Array>(m_InputValues->NodeTypesDataPath)->getDataStoreRef();
  nodeTypes.resizeTuples({numVertices});

  // NodeTypes buffer -- filled during Phase 2B, modified during Phase 3A,
  // then flushed once via copyFromBuffer before triangle generation.
  std::vector<int8> nodeTypesBuf(numVertices, 0);

  // -------------------------------------------------------------------------
  // 6. Phase 2A: Relaxation (optional -- only if smoothing is requested).
  //
  // This reimplements MMSurfaceNet::relax() using the O(surface) m_Vertices
  // and m_CellToVertex data structures instead of the O(volume) MMCellMap.
  //
  // For each surface vertex, the relaxation:
  //   1. Finds up to 6 face-connected neighbors via m_CellToVertex lookup
  //   2. Computes the average neighbor position (in cell-local coordinates)
  //   3. Blends current position toward the average: p' = (1-alpha)*p + alpha*avg
  //   4. Clamps to [0.5 - maxDist, 0.5 + maxDist] to keep vertices near cell centers
  //
  // The face participation rule depends on vertex type:
  //   - SurfaceVertex: participates if the face has any crossing
  //   - JunctionVertex: participates only if the face has a junction crossing
  //
  // Face offsets define the 6 directions to check for neighbors.
  // -------------------------------------------------------------------------
  static constexpr int32 k_FaceOffsets[6][3] = {
      {-1, 0, 0}, // LeftFace
      {+1, 0, 0}, // RightFace
      {0, -1, 0}, // BackFace
      {0, +1, 0}, // FrontFace
      {0, 0, -1}, // BottomFace
      {0, 0, +1}  // TopFace
  };

  // Local position buffer: each vertex starts at (0.5, 0.5, 0.5) which is the
  // cell center in local coordinates. Smoothing modifies these in-place.
  // After smoothing, Phase 2B transforms them to world coordinates.
  std::vector<float32> localPos(3 * numVertices);
  for(usize v = 0; v < numVertices; v++)
  {
    localPos[v * 3 + 0] = 0.5f;
    localPos[v * 3 + 1] = 0.5f;
    localPos[v * 3 + 2] = 0.5f;
  }

  if(m_InputValues->ApplySmoothing)
  {
    const float32 alpha = m_InputValues->RelaxationFactor;
    const float32 maxDist = m_InputValues->MaxDistanceFromVoxel;
    const float32 minClamp = 0.5f - maxDist;
    const float32 maxClamp = 0.5f + maxDist;

    const usize totalSmoothingIterations = static_cast<usize>(m_InputValues->SmoothingIterations);
    for(int32 iter = 0; iter < m_InputValues->SmoothingIterations; iter++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      for(usize v = 0; v < numVertices; v++)
      {
        const auto& vi = m_Vertices[v];
        const int32 cellI = vi.cellIndex[0];
        const int32 cellJ = vi.cellIndex[1];
        const int32 cellK = vi.cellIndex[2];

        int32 numNeighbors = 0;
        float32 avgP[3] = {0.0f, 0.0f, 0.0f};

        for(MMCellFlag::Face face = MMCellFlag::Face::LeftFace; face <= MMCellFlag::Face::TopFace; ++face)
        {
          // Determine whether this face participates based on vertex type
          bool participates = false;
          if(vi.flag.vertexType() == MMCellFlag::VertexType::SurfaceVertex)
          {
            participates = (vi.flag.faceCrossingType(face) != MMCellFlag::FaceCrossingType::NoFaceCrossing);
          }
          else
          {
            participates = (vi.flag.faceCrossingType(face) == MMCellFlag::FaceCrossingType::JunctionFaceCrossing);
          }

          if(!participates)
          {
            continue;
          }

          const int32 faceIdx = static_cast<int32>(face);
          const int32 nbrI = cellI + k_FaceOffsets[faceIdx][0];
          const int32 nbrJ = cellJ + k_FaceOffsets[faceIdx][1];
          const int32 nbrK = cellK + k_FaceOffsets[faceIdx][2];

          // Look up neighbor vertex in the hash map
          const uint64 nbrKey = packCellKey(nbrI, nbrJ, nbrK, paddedX, paddedXY);
          auto it = m_CellToVertex.find(nbrKey);

          float32 nbrPosX = 0.0f, nbrPosY = 0.0f, nbrPosZ = 0.0f;
          if(it != m_CellToVertex.end())
          {
            const usize nbrVtxIdx = it->second;
            nbrPosX = localPos[nbrVtxIdx * 3 + 0];
            nbrPosY = localPos[nbrVtxIdx * 3 + 1];
            nbrPosZ = localPos[nbrVtxIdx * 3 + 2];
          }
          else
          {
            // Neighbor cell has no vertex — use default position (0.5)
            nbrPosX = 0.5f;
            nbrPosY = 0.5f;
            nbrPosZ = 0.5f;
          }

          // Accumulate: neighbor position + offset from current cell to neighbor cell
          avgP[0] += nbrPosX + static_cast<float32>(nbrI - cellI);
          avgP[1] += nbrPosY + static_cast<float32>(nbrJ - cellJ);
          avgP[2] += nbrPosZ + static_cast<float32>(nbrK - cellK);
          numNeighbors++;
        }

        // Blend current position with average neighbor position
        if(numNeighbors > 0)
        {
          avgP[0] /= static_cast<float32>(numNeighbors);
          avgP[1] /= static_cast<float32>(numNeighbors);
          avgP[2] /= static_cast<float32>(numNeighbors);

          float32 x = (1.0f - alpha) * localPos[v * 3 + 0] + alpha * avgP[0];
          x = std::clamp(x, minClamp, maxClamp);
          localPos[v * 3 + 0] = x;

          float32 y = (1.0f - alpha) * localPos[v * 3 + 1] + alpha * avgP[1];
          y = std::clamp(y, minClamp, maxClamp);
          localPos[v * 3 + 1] = y;

          float32 z = (1.0f - alpha) * localPos[v * 3 + 2] + alpha * avgP[2];
          z = std::clamp(z, minClamp, maxClamp);
          localPos[v * 3 + 2] = z;
        }
      }
    }
  }

  // -------------------------------------------------------------------------
  // 7. Phase 2B: Transform vertex positions to world coordinates and assign
  //    NodeTypes.
  //
  // Converts cell-local coordinates (where 0.5 = cell center) to world
  // coordinates using the formula:
  //   worldPos = voxelSize * (cellIndex + localPos) + origin - halfVoxelOffset
  //
  // The halfVoxelOffset accounts for the fact that padded cell index 1
  // corresponds to the first NX voxel, whose center is at origin + 0.5*spacing.
  //
  // Node types are derived from MMCellFlag::numJunctions(): the number of
  // distinct material junctions at the vertex.
  //
  // Note: the Z component of halfVoxelOffset intentionally uses voxelSize[1]
  // to match the original SurfaceNetsDirect.cpp behavior (line 155).
  // -------------------------------------------------------------------------
  auto voxelSize = imageGeom.getSpacing();
  auto origin = imageGeom.getOrigin();
  const Point3Df halfVoxelOffset(0.5f * voxelSize[0], 0.5f * voxelSize[1], 0.5f * voxelSize[1]);

  // Build vertex positions in localPos (reusing the smoothing buffer) and
  // nodeTypes in nodeTypesBuf, then flush both with bulk copyFromBuffer.
  for(usize v = 0; v < numVertices; v++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const auto& vi = m_Vertices[v];

    localPos[v * 3 + 0] = voxelSize[0] * (static_cast<float32>(vi.cellIndex[0]) + localPos[v * 3 + 0]) + origin[0] - halfVoxelOffset[0];
    localPos[v * 3 + 1] = voxelSize[1] * (static_cast<float32>(vi.cellIndex[1]) + localPos[v * 3 + 1]) + origin[1] - halfVoxelOffset[1];
    localPos[v * 3 + 2] = voxelSize[2] * (static_cast<float32>(vi.cellIndex[2]) + localPos[v * 3 + 2]) + origin[2] - halfVoxelOffset[2];

    nodeTypesBuf[v] = static_cast<int8>(vi.flag.numJunctions());
  }

  // -------------------------------------------------------------------------
  // 8. Phase 3A: First pass -- count triangles.
  //
  // Iterates all surface vertices and checks 3 edges per cell:
  //   - BackBottomEdge: between this cell and (i+1, j, k)
  //   - LeftBottomEdge: between this cell and (i, j+1, k)
  //   - LeftBackEdge:   between this cell and (i, j, k+1)
  //
  // Each edge crossing produces a quad = 2 triangles. The 4 quad vertices
  // are the current cell's vertex plus 3 neighbors found via lookupVertex().
  //
  // This pass also marks boundary vertices via HandlePadding() when one of
  // the quad labels is Padding (exterior of volume).
  //
  // After this pass, vertex positions and node types are flushed to their
  // DataStores via copyFromBuffer() since they are now finalized.
  // -------------------------------------------------------------------------
  usize triangleCount = 0;
  std::array<usize, 2> quadNxArrayIndices = {0, 0};

  for(usize v = 0; v < numVertices; v++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto& vi = m_Vertices[v];
    const int32 cellI = vi.cellIndex[0];
    const int32 cellJ = vi.cellIndex[1];
    const int32 cellK = vi.cellIndex[2];

    // BackBottomEdge
    if(vi.flag.isEdgeCrossing(MMCellFlag::Edge::BackBottomEdge))
    {
      std::array<usize, 4> vertexIndices = {v, lookupVertex(m_CellToVertex, cellI, cellJ - 1, cellK, paddedX, paddedXY), lookupVertex(m_CellToVertex, cellI, cellJ - 1, cellK - 1, paddedX, paddedXY),
                                            lookupVertex(m_CellToVertex, cellI, cellJ, cellK - 1, paddedX, paddedXY)};

      std::array<LabelType, 2> quadLabels;
      quadLabels[0] = edgeCellLabel(cellI, cellJ, cellK, paddedX, paddedY, paddedZ, dimX, dimY, featureIdsStore);
      quadLabels[1] = edgeCellLabel(cellI + 1, cellJ, cellK, paddedX, paddedY, paddedZ, dimX, dimY, featureIdsStore);

      if(quadLabels[0] == MMSurfaceNet::Padding || quadLabels[1] == MMSurfaceNet::Padding)
      {
        HandlePadding(vertexIndices, nodeTypesBuf);
      }
      triangleCount += 2;
    }

    // LeftBottomEdge
    if(vi.flag.isEdgeCrossing(MMCellFlag::Edge::LeftBottomEdge))
    {
      std::array<usize, 4> vertexIndices = {v, lookupVertex(m_CellToVertex, cellI, cellJ, cellK - 1, paddedX, paddedXY), lookupVertex(m_CellToVertex, cellI - 1, cellJ, cellK - 1, paddedX, paddedXY),
                                            lookupVertex(m_CellToVertex, cellI - 1, cellJ, cellK, paddedX, paddedXY)};

      std::array<LabelType, 2> quadLabels;
      quadLabels[0] = edgeCellLabel(cellI, cellJ, cellK, paddedX, paddedY, paddedZ, dimX, dimY, featureIdsStore);
      quadLabels[1] = edgeCellLabel(cellI, cellJ + 1, cellK, paddedX, paddedY, paddedZ, dimX, dimY, featureIdsStore);

      if(quadLabels[0] == MMSurfaceNet::Padding || quadLabels[1] == MMSurfaceNet::Padding)
      {
        HandlePadding(vertexIndices, nodeTypesBuf);
      }
      triangleCount += 2;
    }

    // LeftBackEdge
    if(vi.flag.isEdgeCrossing(MMCellFlag::Edge::LeftBackEdge))
    {
      std::array<usize, 4> vertexIndices = {v, lookupVertex(m_CellToVertex, cellI - 1, cellJ, cellK, paddedX, paddedXY), lookupVertex(m_CellToVertex, cellI - 1, cellJ - 1, cellK, paddedX, paddedXY),
                                            lookupVertex(m_CellToVertex, cellI, cellJ - 1, cellK, paddedX, paddedXY)};

      std::array<LabelType, 2> quadLabels;
      quadLabels[0] = edgeCellLabel(cellI, cellJ, cellK, paddedX, paddedY, paddedZ, dimX, dimY, featureIdsStore);
      quadLabels[1] = edgeCellLabel(cellI, cellJ, cellK + 1, paddedX, paddedY, paddedZ, dimX, dimY, featureIdsStore);

      if(quadLabels[0] == MMSurfaceNet::Padding || quadLabels[1] == MMSurfaceNet::Padding)
      {
        HandlePadding(vertexIndices, nodeTypesBuf);
      }
      triangleCount += 2;
    }
  }

  // Flush buffered vertex positions and nodeTypes to their DataStores
  verticesStore.copyFromBuffer(0, nonstd::span<const float32>(localPos.data(), localPos.size()));
  nodeTypes.copyFromBuffer(0, nonstd::span<const int8>(nodeTypesBuf.data(), nodeTypesBuf.size()));

  // -------------------------------------------------------------------------
  // 9. Phase 3B: Resize face arrays
  // -------------------------------------------------------------------------
  triangleGeom.resizeFaceList(triangleCount);
  triangleGeom.getFaceAttributeMatrix()->resizeTuples({triangleCount});

  auto& faceLabels = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef();
  faceLabels.resizeTuples({triangleCount});

  // -------------------------------------------------------------------------
  // 10. Phase 3C: TupleTransfer setup
  // -------------------------------------------------------------------------
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

  // -------------------------------------------------------------------------
  // 11. Phase 3D: Second pass -- generate triangles.
  //
  // Same edge iteration as Phase 3A, but now writes triangle data. All
  // output is accumulated in local buffers:
  //   - triConnBuf: triangle connectivity (3 MeshIndexType per triangle)
  //   - faceLabelBuf: face labels (2 int32 per triangle)
  //   - ttArgsBuf: TupleTransfer arguments (1 per triangle)
  //
  // These are flushed in one bulk write at the end (section 12b). This
  // avoids per-element writes to OOC DataStores.
  //
  // Each edge crossing produces a quad from 4 vertices, which is split into
  // 2 triangles using getQuadTriangleIDs() (minimal-area diagonal selection).
  // Face labels are ordered so the smaller label is first.
  // -------------------------------------------------------------------------
  using MeshIndexType = IGeometry::MeshIndexType;
  auto& facesStore = triangleGeom.getFacesRef().getDataStoreRef();

  // Pre-allocate output buffers for all triangles (known count from Phase 3A)
  std::vector<MeshIndexType> triConnBuf;
  std::vector<int32> faceLabelBuf;
  std::vector<SurfaceNetsTransferData> ttArgsBuf;
  triConnBuf.reserve(triangleCount * 3);
  faceLabelBuf.reserve(triangleCount * 2);
  ttArgsBuf.reserve(triangleCount);

  usize faceIndex = 0;
  std::array<usize, 6> triangleVtxIDs = {0, 0, 0, 0, 0, 0};
  std::array<usize, 4> vertexIndices = {0, 0, 0, 0};
  std::array<LabelType, 2> quadLabels = {0, 0};
  std::array<VertexData, 4> vData{};

  for(usize v = 0; v < numVertices; v++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const auto& vi = m_Vertices[v];
    const int32 cellI = vi.cellIndex[0];
    const int32 cellJ = vi.cellIndex[1];
    const int32 cellK = vi.cellIndex[2];

    // Back-bottom edge
    if(vi.flag.isEdgeCrossing(MMCellFlag::Edge::BackBottomEdge))
    {
      vertexIndices[0] = v;
      vertexIndices[1] = lookupVertex(m_CellToVertex, cellI, cellJ - 1, cellK, paddedX, paddedXY);
      vertexIndices[2] = lookupVertex(m_CellToVertex, cellI, cellJ - 1, cellK - 1, paddedX, paddedXY);
      vertexIndices[3] = lookupVertex(m_CellToVertex, cellI, cellJ, cellK - 1, paddedX, paddedXY);

      quadLabels[0] = edgeCellLabel(cellI, cellJ, cellK, paddedX, paddedY, paddedZ, dimX, dimY, featureIdsStore);
      quadLabels[1] = edgeCellLabel(cellI + 1, cellJ, cellK, paddedX, paddedY, paddedZ, dimX, dimY, featureIdsStore);

      quadNxArrayIndices[0] = edgeCellNxIndex(cellI, cellJ, cellK, paddedX, paddedY, paddedZ, dimX, dimY, dimZ);
      quadNxArrayIndices[1] = edgeCellNxIndex(cellI + 1, cellJ, cellK, paddedX, paddedY, paddedZ, dimX, dimY, dimZ);

      vData[0] = {vertexIndices[0], {0.0f, 0.0f, 0.0f}};
      vData[1] = {vertexIndices[1], {0.0f, 0.0f, 0.0f}};
      vData[2] = {vertexIndices[2], {0.0f, 0.0f, 0.0f}};
      vData[3] = {vertexIndices[3], {0.0f, 0.0f, 0.0f}};

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

      // Triangle 1
      triConnBuf.push_back(triangleVtxIDs[0]);
      triConnBuf.push_back(triangleVtxIDs[1]);
      triConnBuf.push_back(triangleVtxIDs[2]);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabelBuf.push_back(quadLabels[0]);
        faceLabelBuf.push_back(quadLabels[1]);
      }
      else
      {
        faceLabelBuf.push_back(quadLabels[1]);
        faceLabelBuf.push_back(quadLabels[0]);
      }
      ttArgsBuf.push_back({faceIndex, quadNxArrayIndices});
      faceIndex++;

      // Triangle 2
      triConnBuf.push_back(triangleVtxIDs[3]);
      triConnBuf.push_back(triangleVtxIDs[4]);
      triConnBuf.push_back(triangleVtxIDs[5]);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabelBuf.push_back(quadLabels[0]);
        faceLabelBuf.push_back(quadLabels[1]);
      }
      else
      {
        faceLabelBuf.push_back(quadLabels[1]);
        faceLabelBuf.push_back(quadLabels[0]);
      }
      ttArgsBuf.push_back({faceIndex, quadNxArrayIndices});
      faceIndex++;
    }

    // Left-bottom edge
    if(vi.flag.isEdgeCrossing(MMCellFlag::Edge::LeftBottomEdge))
    {
      vertexIndices[0] = v;
      vertexIndices[1] = lookupVertex(m_CellToVertex, cellI, cellJ, cellK - 1, paddedX, paddedXY);
      vertexIndices[2] = lookupVertex(m_CellToVertex, cellI - 1, cellJ, cellK - 1, paddedX, paddedXY);
      vertexIndices[3] = lookupVertex(m_CellToVertex, cellI - 1, cellJ, cellK, paddedX, paddedXY);

      quadLabels[0] = edgeCellLabel(cellI, cellJ, cellK, paddedX, paddedY, paddedZ, dimX, dimY, featureIdsStore);
      quadLabels[1] = edgeCellLabel(cellI, cellJ + 1, cellK, paddedX, paddedY, paddedZ, dimX, dimY, featureIdsStore);

      quadNxArrayIndices[0] = edgeCellNxIndex(cellI, cellJ, cellK, paddedX, paddedY, paddedZ, dimX, dimY, dimZ);
      quadNxArrayIndices[1] = edgeCellNxIndex(cellI, cellJ + 1, cellK, paddedX, paddedY, paddedZ, dimX, dimY, dimZ);

      vData[0] = {vertexIndices[0], {0.0f, 0.0f, 0.0f}};
      vData[1] = {vertexIndices[1], {0.0f, 0.0f, 0.0f}};
      vData[2] = {vertexIndices[2], {0.0f, 0.0f, 0.0f}};
      vData[3] = {vertexIndices[3], {0.0f, 0.0f, 0.0f}};

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

      // Triangle 1
      triConnBuf.push_back(triangleVtxIDs[0]);
      triConnBuf.push_back(triangleVtxIDs[1]);
      triConnBuf.push_back(triangleVtxIDs[2]);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabelBuf.push_back(quadLabels[0]);
        faceLabelBuf.push_back(quadLabels[1]);
      }
      else
      {
        faceLabelBuf.push_back(quadLabels[1]);
        faceLabelBuf.push_back(quadLabels[0]);
      }
      ttArgsBuf.push_back({faceIndex, quadNxArrayIndices});
      faceIndex++;

      // Triangle 2
      triConnBuf.push_back(triangleVtxIDs[3]);
      triConnBuf.push_back(triangleVtxIDs[4]);
      triConnBuf.push_back(triangleVtxIDs[5]);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabelBuf.push_back(quadLabels[0]);
        faceLabelBuf.push_back(quadLabels[1]);
      }
      else
      {
        faceLabelBuf.push_back(quadLabels[1]);
        faceLabelBuf.push_back(quadLabels[0]);
      }
      ttArgsBuf.push_back({faceIndex, quadNxArrayIndices});
      faceIndex++;
    }

    // Left-back edge
    if(vi.flag.isEdgeCrossing(MMCellFlag::Edge::LeftBackEdge))
    {
      vertexIndices[0] = v;
      vertexIndices[1] = lookupVertex(m_CellToVertex, cellI - 1, cellJ, cellK, paddedX, paddedXY);
      vertexIndices[2] = lookupVertex(m_CellToVertex, cellI - 1, cellJ - 1, cellK, paddedX, paddedXY);
      vertexIndices[3] = lookupVertex(m_CellToVertex, cellI, cellJ - 1, cellK, paddedX, paddedXY);

      quadLabels[0] = edgeCellLabel(cellI, cellJ, cellK, paddedX, paddedY, paddedZ, dimX, dimY, featureIdsStore);
      quadLabels[1] = edgeCellLabel(cellI, cellJ, cellK + 1, paddedX, paddedY, paddedZ, dimX, dimY, featureIdsStore);

      quadNxArrayIndices[0] = edgeCellNxIndex(cellI, cellJ, cellK, paddedX, paddedY, paddedZ, dimX, dimY, dimZ);
      quadNxArrayIndices[1] = edgeCellNxIndex(cellI, cellJ, cellK + 1, paddedX, paddedY, paddedZ, dimX, dimY, dimZ);

      vData[0] = {vertexIndices[0], {0.0f, 0.0f, 0.0f}};
      vData[1] = {vertexIndices[1], {0.0f, 0.0f, 0.0f}};
      vData[2] = {vertexIndices[2], {0.0f, 0.0f, 0.0f}};
      vData[3] = {vertexIndices[3], {0.0f, 0.0f, 0.0f}};

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

      // Triangle 1
      triConnBuf.push_back(triangleVtxIDs[0]);
      triConnBuf.push_back(triangleVtxIDs[1]);
      triConnBuf.push_back(triangleVtxIDs[2]);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabelBuf.push_back(quadLabels[0]);
        faceLabelBuf.push_back(quadLabels[1]);
      }
      else
      {
        faceLabelBuf.push_back(quadLabels[1]);
        faceLabelBuf.push_back(quadLabels[0]);
      }
      ttArgsBuf.push_back({faceIndex, quadNxArrayIndices});
      faceIndex++;

      // Triangle 2
      triConnBuf.push_back(triangleVtxIDs[3]);
      triConnBuf.push_back(triangleVtxIDs[4]);
      triConnBuf.push_back(triangleVtxIDs[5]);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabelBuf.push_back(quadLabels[0]);
        faceLabelBuf.push_back(quadLabels[1]);
      }
      else
      {
        faceLabelBuf.push_back(quadLabels[1]);
        faceLabelBuf.push_back(quadLabels[0]);
      }
      ttArgsBuf.push_back({faceIndex, quadNxArrayIndices});
      faceIndex++;
    }
  }

  // -------------------------------------------------------------------------
  // 12. Phase 3E: FaceLabels fixup -- replace 0 with -1 in the local buffer
  // -------------------------------------------------------------------------
  for(auto& label : faceLabelBuf)
  {
    if(label == 0)
    {
      label = -1;
    }
  }

  // -------------------------------------------------------------------------
  // 12b. Flush buffered triangle connectivity, face labels, and TupleTransfer
  // -------------------------------------------------------------------------
  if(!triConnBuf.empty())
  {
    facesStore.copyFromBuffer(0, nonstd::span<const MeshIndexType>(triConnBuf.data(), triConnBuf.size()));
    faceLabels.copyFromBuffer(0, nonstd::span<const int32>(faceLabelBuf.data(), faceLabelBuf.size()));
    for(const auto& tupleTransferFunction : tupleTransferFunctions)
    {
      tupleTransferFunction->surfaceNetsTransferBatch(nonstd::span<const SurfaceNetsTransferData>(ttArgsBuf.data(), ttArgsBuf.size()));
    }
  }

  // -------------------------------------------------------------------------
  // 13. Phase 3F: Winding repair
  // -------------------------------------------------------------------------
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

    // Purge connectivity
    m_DataStructure.removeData(triangleGeom.getElementContainingVertId().value());
    m_DataStructure.removeData(triangleGeom.getElementNeighborsId().value());
  }

  return windingResult;
}
