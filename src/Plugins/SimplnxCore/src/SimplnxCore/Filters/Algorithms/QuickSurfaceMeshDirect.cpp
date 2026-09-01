/**
 * @file QuickSurfaceMeshDirect.cpp
 * @brief Implements in-memory QuickSurfaceMesh execution.
 *
 * Direct element access and a full dual-grid node map give reference topology
 * for QuickSurfaceMeshScanline. The node map uses one entry per grid corner, so
 * memory scales with volume.
 *
 * Correction uses a fixed random sequence. Counting assigns exact output sizes.
 * Generation creates faces, labels, node types, and transferred data. Optional
 * winding repair builds transient neighbor topology after generation.
 */

#include "QuickSurfaceMeshDirect.hpp"

#include "QuickSurfaceMesh.hpp"
#include "TupleTransfer.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <fmt/format.h>
#include <random>
#include <set>

using namespace nx::core;

namespace
{
// Match QuickSurfaceMeshScanline's fixed random-number seed and draw order for correction parity.
constexpr float64 k_RangeMin = 0.0;
constexpr float64 k_RangeMax = 1.0;
constexpr std::mt19937_64::result_type k_Seed = 3412341234123412;
std::mt19937_64 generator(k_Seed);
std::uniform_real_distribution<> distribution(k_RangeMin, k_RangeMax);

/**
 * @brief Writes one dual-grid vertex position.
 * @param grid Grid geometry that supplies plane coordinates.
 * @param x Grid-plane X coordinate.
 * @param y Grid-plane Y coordinate.
 * @param z Grid-plane Z coordinate.
 * @param verts Vertex scalar store.
 * @param nodeIndex Flat scalar offset of the coordinate X value.
 * @pre grid is not null and verts has three values beginning at nodeIndex.
 *
 * getPlaneCoords() returns physical coordinates. The helper converts them to
 * VertexStore::value_type before storage.
 */
void GetGridCoordinates(const IGridGeometry* grid, usize x, usize y, usize z, QuickSurfaceMeshDirect::VertexStore& verts, IGeometry::MeshIndexType nodeIndex)
{
  nx::core::Point3D<float64> tmpCoords = grid->getPlaneCoords(x, y, z);
  verts[nodeIndex] = static_cast<QuickSurfaceMeshDirect::VertexStore::value_type>(tmpCoords[0]);
  verts[nodeIndex + 1] = static_cast<QuickSurfaceMeshDirect::VertexStore::value_type>(tmpCoords[1]);
  verts[nodeIndex + 2] = static_cast<QuickSurfaceMeshDirect::VertexStore::value_type>(tmpCoords[2]);
}

/** @brief Returns true when the selected mode omits a background-backed wall face. */
bool SkipWallFace(ChoicesParameter::ValueType mode, const Int32AbstractDataStore& featureIds, usize point)
{
  return mode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && featureIds[point] == 0;
}

// -----------------------------------------------------------------------------
/**
 * @brief Resolves a Case 1 body-diagonal conflict.
 * @param featureIds Feature ID store to modify.
 * @param v1 First mutable voxel index.
 * @param v2 First replacement source for v1.
 * @param v3 Second replacement source for v1.
 * @param v4 First replacement source for v6.
 * @param v5 Second replacement source for v6.
 * @param v6 Second mutable voxel index.
 *
 * Four equal-width random intervals select one assignment. The case ordering
 * and draw sequence must match QuickSurfaceMeshScanline.
 */
void FlipProblemVoxelCase1(Int32AbstractDataStore& featureIds, QuickSurfaceMeshDirect::MeshIndexType v1, QuickSurfaceMeshDirect::MeshIndexType v2, QuickSurfaceMeshDirect::MeshIndexType v3,
                           QuickSurfaceMeshDirect::MeshIndexType v4, QuickSurfaceMeshDirect::MeshIndexType v5, QuickSurfaceMeshDirect::MeshIndexType v6)
{
  auto val = static_cast<float32>(distribution(generator));

  if(val < 0.25f)
  {
    featureIds[v6] = featureIds[v4];
  }
  else if(val < 0.5f)
  {
    featureIds[v6] = featureIds[v5];
  }
  else if(val < 0.75f)
  {
    featureIds[v1] = featureIds[v2];
  }
  else
  {
    featureIds[v1] = featureIds[v3];
  }
}

/**
 * @brief Resolves a Case 2 edge-diagonal conflict with the current Direct sequence.
 * @param featureIds Feature ID store to modify.
 * @param v1 First index in the Case 2 call order.
 * @param v2 Second index in the Case 2 call order.
 * @param v3 Third index in the Case 2 call order.
 * @param v4 Fourth index in the Case 2 call order.
 *
 * The first conditional handles values below 0.375. The following independent
 * conditional handles values below 0.5 or the later ranges. Values below 0.375
 * perform two assignments. The 0.25-to-0.375 assignment to v2 is overwritten.
 * This sequence does not select eight equal choices.
 * QuickSurfaceMeshScanline preserves the same behavior.
 */
void FlipProblemVoxelCase2(Int32AbstractDataStore& featureIds, QuickSurfaceMeshDirect::MeshIndexType v1, QuickSurfaceMeshDirect::MeshIndexType v2, QuickSurfaceMeshDirect::MeshIndexType v3,
                           QuickSurfaceMeshDirect::MeshIndexType v4)
{
  auto val = static_cast<float32>(distribution(generator));

  if(val < 0.125f)
  {
    featureIds[v1] = featureIds[v2];
  }
  else if(val < 0.25f)
  {
    featureIds[v1] = featureIds[v3];
  }
  else if(val < 0.375f)
  {
    featureIds[v2] = featureIds[v1];
  }
  if(val < 0.5f)
  {
    featureIds[v2] = featureIds[v4];
  }
  else if(val < 0.625f)
  {
    featureIds[v3] = featureIds[v1];
  }
  else if(val < 0.75f)
  {
    featureIds[v3] = featureIds[v4];
  }
  else if(val < 0.875f)
  {
    featureIds[v4] = featureIds[v2];
  }
  else
  {
    featureIds[v4] = featureIds[v3];
  }
}

/**
 * @brief Resolves a Case 3 isolated-voxel conflict.
 * @param featureIds Feature ID store to modify.
 * @param v1 Source voxel index.
 * @param v2 First destination voxel index.
 * @param v3 Second destination voxel index.
 *
 * Equal-width intervals copy v1 to v2 or v3. The draw sequence must match
 * QuickSurfaceMeshScanline.
 */
void FlipProblemVoxelCase3(Int32AbstractDataStore& featureIds, QuickSurfaceMeshDirect::MeshIndexType v1, QuickSurfaceMeshDirect::MeshIndexType v2, QuickSurfaceMeshDirect::MeshIndexType v3)
{
  auto val = static_cast<float32>(distribution(generator));

  if(val < 0.5f)
  {
    featureIds[v2] = featureIds[v1];
  }
  else
  {
    featureIds[v3] = featureIds[v1];
  }
}
} // namespace

QuickSurfaceMeshDirect::QuickSurfaceMeshDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               const QuickSurfaceMeshInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
  generator.seed(k_Seed);
}

QuickSurfaceMeshDirect::~QuickSurfaceMeshDirect() noexcept = default;

Result<> QuickSurfaceMeshDirect::operator()()
{
  const auto& featureIdsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  Result<> sentinelCheck = MeshingUtilities::ValidateFeatureIdsAgainstSentinels(featureIdsStore, m_InputValues->FeatureIdsArrayPath, false, m_ShouldCancel, m_MessageHandler);
  if(sentinelCheck.invalid())
  {
    return sentinelCheck;
  }

  auto& grid = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  SizeVec3 udims = grid.getDimensions();

  usize xP = udims[0];
  usize yP = udims[1];
  usize zP = udims[2];

  // Direct execution keeps one ID for every dual-grid corner. The scanline path
  // uses rolling planes when a full-volume map does not fit resident memory.
  usize possibleNumNodes = (xP + 1) * (yP + 1) * (zP + 1);
  std::vector<MeshIndexType> nodeIds(possibleNumNodes, std::numeric_limits<usize>::max());

  MeshIndexType nodeCount = 0;
  MeshIndexType triangleCount = 0;
  MeshIndexType suppressedFaceCount = 0;

  // Resolve ambiguities before counting so all later topology uses final Feature IDs.
  if(m_InputValues->FixProblemVoxels)
  {
    correctProblemVoxels();
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  determineActiveNodes(nodeIds, nodeCount, triangleCount, suppressedFaceCount);
  if(m_ShouldCancel)
  {
    return {};
  }

  // Count first so generation writes final-size geometry and attribute arrays.
  ShapeType tupleShape = {triangleCount};
  triangleGeom.resizeFaceList(triangleCount);
  triangleGeom.resizeVertexList(nodeCount);
  triangleGeom.getFaceAttributeMatrix()->resizeTuples(tupleShape);
  triangleGeom.getVertexAttributeMatrix()->resizeTuples({nodeCount});

  for(const auto& dataPath : m_InputValues->CreatedDataArrayPaths)
  {
    Result<> result = nx::core::ResizeAndReplaceDataArray(m_DataStructure, dataPath, tupleShape, nx::core::IDataAction::Mode::Execute);
  }

  createNodesAndTriangles(nodeIds, nodeCount, triangleCount);
  if(m_ShouldCancel)
  {
    return {};
  }

  Result<> windingResult = {};
  if(m_InputValues->RepairTriangleWinding)
  {
    // Direct winding repair builds transient neighbor topology. The scanline path
    // can use external sorting when any dispatch target is out of core.
    m_MessageHandler(IFilter::Message::Type::Info, "Generating Connectivity and Triangle Neighbors...");
    triangleGeom.findElementNeighbors(true);
    const auto optionalId = triangleGeom.getElementNeighborsId();
    if(!optionalId.has_value())
    {
      return MakeErrorResult(-56341, fmt::format("Unable to generate the connectivity list for {} geometry.", triangleGeom.getName()));
    }
    const auto& connectivity = m_DataStructure.getDataRefAs<IGeometry::ElementDynamicList>(optionalId.value());

    m_MessageHandler(IFilter::Message::Type::Info, "Repairing Windings...");
    windingResult = MeshingUtilities::RepairTriangleWinding(triangleGeom.getFaces()->getDataStoreRef(), connectivity,
                                                            m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef(), m_ShouldCancel, m_MessageHandler);

    // Connectivity lists are temporary winding scratch. Remove them after repair.
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

void QuickSurfaceMeshDirect::correctProblemVoxels()
{
  m_MessageHandler(IFilter::Message::Type::Info, "Correcting Problem Voxels");

  auto* grid = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  SizeVec3 udims = grid->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];

  // v1-v4 use Z=k-1 and v5-v8 use Z=k. Each plane orders corners as
  // (-X,-Y), (+X,-Y), (-X,+Y), and (+X,+Y) within the current block.
  // This mapping must match the scanline Case 1, Case 2, and Case 3 calls.
  MeshIndexType v1 = 0, v2 = 0, v3 = 0, v4 = 0;
  MeshIndexType v5 = 0, v6 = 0, v7 = 0, v8 = 0;

  int32 f1 = 0, f2 = 0, f3 = 0, f4 = 0;
  int32 f5 = 0, f6 = 0, f7 = 0, f8 = 0;

  // Plane and row offsets form Direct-compatible flat voxel indices.
  MeshIndexType row1 = 0, row2 = 0;
  MeshIndexType plane1 = 0, plane2 = 0;

  MeshIndexType count = 1;
  MeshIndexType iter = 0;
  // A correction can expose another conflict, so repeat at most 20 passes.
  while(count > 0 && iter < 20)
  {
    if(m_ShouldCancel)
    {
      return;
    }
    iter++;
    count = 0;

    for(MeshIndexType k = 1; k < zP; k++)
    {
      plane1 = (k - 1) * xP * yP;
      plane2 = k * xP * yP;
      for(MeshIndexType j = 1; j < yP; j++)
      {
        row1 = (j - 1) * xP;
        row2 = j * xP;
        for(MeshIndexType i = 1; i < xP; i++)
        {
          v1 = plane1 + row1 + i - 1;
          v2 = plane1 + row1 + i;
          v3 = plane1 + row2 + i - 1;
          v4 = plane1 + row2 + i;
          v5 = plane2 + row1 + i - 1;
          v6 = plane2 + row1 + i;
          v7 = plane2 + row2 + i - 1;
          v8 = plane2 + row2 + i;

          f1 = featureIds[v1];
          f2 = featureIds[v2];
          f3 = featureIds[v3];
          f4 = featureIds[v4];
          f5 = featureIds[v5];
          f6 = featureIds[v6];
          f7 = featureIds[v7];
          f8 = featureIds[v8];

          // Preserve Case 1, Case 2, then Case 3 order. Each accepted case consumes
          // the next random value, and the sequence defines scanline parity.
          if(f1 == f8 && f1 != f2 && f1 != f3 && f1 != f4 && f1 != f5 && f1 != f6 && f1 != f7)
          {
            ::FlipProblemVoxelCase1(featureIds, v1, v2, v3, v6, v7, v8);
            count++;
          }
          if(f2 == f7 && f2 != f1 && f2 != f3 && f2 != f4 && f2 != f5 && f2 != f6 && f2 != f8)
          {
            ::FlipProblemVoxelCase1(featureIds, v2, v1, v4, v5, v8, v7);
            count++;
          }
          if(f3 == f6 && f3 != f1 && f3 != f2 && f3 != f4 && f3 != f5 && f3 != f7 && f3 != f8)
          {
            ::FlipProblemVoxelCase1(featureIds, v3, v1, v4, v5, v8, v6);
            count++;
          }
          if(f4 == f5 && f4 != f1 && f4 != f2 && f4 != f3 && f4 != f6 && f4 != f7 && f4 != f8)
          {
            ::FlipProblemVoxelCase1(featureIds, v4, v2, v3, v6, v7, v5);
            count++;
          }
          if(f1 == f6 && f1 != f2 && f1 != f5)
          {
            ::FlipProblemVoxelCase2(featureIds, v1, v2, v5, v6);
            count++;
          }
          if(f2 == f5 && f2 != f1 && f2 != f6)
          {
            ::FlipProblemVoxelCase2(featureIds, v2, v1, v6, v5);
            count++;
          }
          if(f3 == f8 && f3 != f4 && f3 != f7)
          {
            ::FlipProblemVoxelCase2(featureIds, v3, v4, v7, v8);
            count++;
          }
          if(f4 == f7 && f4 != f3 && f4 != f8)
          {
            ::FlipProblemVoxelCase2(featureIds, v4, v3, v8, v7);
            count++;
          }
          if(f1 == f7 && f1 != f3 && f1 != f5)
          {
            ::FlipProblemVoxelCase2(featureIds, v1, v3, v5, v7);
            count++;
          }
          if(f3 == f5 && f3 != f1 && f3 != f7)
          {
            ::FlipProblemVoxelCase2(featureIds, v3, v1, v7, v5);
            count++;
          }
          if(f2 == f8 && f2 != f4 && f2 != f6)
          {
            ::FlipProblemVoxelCase2(featureIds, v2, v4, v6, v8);
            count++;
          }
          if(f4 == f6 && f4 != f2 && f4 != f8)
          {
            ::FlipProblemVoxelCase2(featureIds, v4, v2, v8, v6);
            count++;
          }
          if(f1 == f4 && f1 != f2 && f1 != f3)
          {
            ::FlipProblemVoxelCase2(featureIds, v1, v2, v3, v4);
            count++;
          }
          if(f2 == f3 && f2 != f1 && f2 != f4)
          {
            ::FlipProblemVoxelCase2(featureIds, v2, v1, v4, v3);
            count++;
          }
          if(f5 == f8 && f5 != f6 && f5 != f7)
          {
            ::FlipProblemVoxelCase2(featureIds, v5, v6, v7, v8);
            count++;
          }
          if(f6 == f7 && f6 != f5 && f6 != f8)
          {
            ::FlipProblemVoxelCase2(featureIds, v6, v5, v8, v7);
            count++;
          }
          if(f2 == f3 && f2 == f4 && f2 == f5 && f2 == f6 && f2 == f7 && f2 != f1 && f2 != f8)
          {
            ::FlipProblemVoxelCase3(featureIds, v2, v1, v8);
            count++;
          }
          if(f1 == f3 && f1 == f4 && f1 == f5 && f1 == f7 && f2 == f8 && f1 != f2 && f1 != f7)
          {
            ::FlipProblemVoxelCase3(featureIds, v1, v2, v7);
            count++;
          }
          if(f1 == f2 && f1 == f4 && f1 == f5 && f1 == f7 && f1 == f8 && f1 != f3 && f1 != f6)
          {
            ::FlipProblemVoxelCase3(featureIds, v1, v3, v6);
            count++;
          }
          if(f1 == f2 && f1 == f3 && f1 == f6 && f1 == f7 && f1 == f8 && f1 != f4 && f1 != f5)
          {
            ::FlipProblemVoxelCase3(featureIds, v1, v4, v5);
            count++;
          }
        }
      }
    }

    std::string ss = fmt::format("Correcting Problem Voxels: Iteration - '{}'; Problem Voxels - '{}'", iter, count);
    m_MessageHandler(IFilter::Message::Type::Info, ss);
  }
}

// -----------------------------------------------------------------------------
/**
 * @brief First pass: counts unique mesh vertices (nodes) and triangles.
 *
 * For each voxel, the algorithm checks six face conditions:
 *   - i==0, i==xP-1: left/right volume boundary faces
 *   - j==0, j==yP-1: front/back volume boundary faces
 *   - k==0, k==zP-1: bottom/top volume boundary faces
 *   - FeatureId differs from +X, +Y, or +Z neighbor: interior feature boundary
 *
 * Each face that produces triangles has 4 dual-grid nodes (corners of the
 * face rectangle). The nodeIds array maps each possible dual-grid node
 * (indexed by its (xP+1)*(yP+1)*(zP+1) linear position) to a sequential
 * vertex ID. Unvisited entries contain max (sentinel value).
 *
 * Two triangles are generated per boundary face, so triangleCount is
 * incremented by 2 for each detected face.
 */
void QuickSurfaceMeshDirect::determineActiveNodes(std::vector<MeshIndexType>& nodeIds, MeshIndexType& nodeCount, MeshIndexType& triangleCount, MeshIndexType& suppressedFaceCount)
{
  m_MessageHandler(IFilter::Message::Type::Info, "Determining active Nodes");

  auto* grid = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  Int32AbstractDataStore& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  SizeVec3 udims = grid->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];

  if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
  {
    for(MeshIndexType k = 0; k < zP; k++)
    {
      for(MeshIndexType j = 0; j < yP; j++)
      {
        for(MeshIndexType i = 0; i < xP; i++)
        {
          const MeshIndexType point = (k * xP * yP) + (j * xP) + i;
          if(featureIds[point] == 0)
          {
            suppressedFaceCount += static_cast<MeshIndexType>(i == 0) + static_cast<MeshIndexType>(i == xP - 1) + static_cast<MeshIndexType>(j == 0) + static_cast<MeshIndexType>(j == yP - 1) +
                                   static_cast<MeshIndexType>(k == 0) + static_cast<MeshIndexType>(k == zP - 1);
          }
        }
      }
    }
  }

  // Linear indices: point = current voxel, neigh1/2/3 = +X, +Y, +Z neighbors
  MeshIndexType point = 0, neigh1 = 0, neigh2 = 0, neigh3 = 0;

  // Four dual-grid corners define each face. The max sentinel marks an unassigned vertex ID.
  MeshIndexType nodeId1 = 0, nodeId2 = 0, nodeId3 = 0, nodeId4 = 0;

  for(MeshIndexType k = 0; k < zP; k++)
  {
    // Check once per Z plane to keep the face loop free of atomic reads.
    if(m_ShouldCancel)
    {
      return;
    }
    for(MeshIndexType j = 0; j < yP; j++)
    {
      for(MeshIndexType i = 0; i < xP; i++)
      {
        point = (k * xP * yP) + (j * xP) + i;
        neigh1 = point + 1;
        neigh2 = point + xP;
        neigh3 = point + (xP * yP);

        if(i == 0 && !SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          if(nodeIds[nodeId1] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId1] = nodeCount;
            nodeCount++;
          }
          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          if(nodeIds[nodeId2] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId2] = nodeCount;
            nodeCount++;
          }
          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          if(nodeIds[nodeId3] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId3] = nodeCount;
            nodeCount++;
          }
          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          if(nodeIds[nodeId4] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId4] = nodeCount;
            nodeCount++;
          }
          triangleCount++;
          triangleCount++;
        }
        if(j == 0 && !SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          if(nodeIds[nodeId1] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId1] = nodeCount;
            nodeCount++;
          }
          nodeId2 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId2] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId2] = nodeCount;
            nodeCount++;
          }
          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          if(nodeIds[nodeId3] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId3] = nodeCount;
            nodeCount++;
          }
          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId4] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId4] = nodeCount;
            nodeCount++;
          }
          triangleCount++;
          triangleCount++;
        }
        if(k == 0 && !SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          if(nodeIds[nodeId1] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId1] = nodeCount;
            nodeCount++;
          }
          nodeId2 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId2] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId2] = nodeCount;
            nodeCount++;
          }
          nodeId3 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          if(nodeIds[nodeId3] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId3] = nodeCount;
            nodeCount++;
          }
          nodeId4 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId4] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId4] = nodeCount;
            nodeCount++;
          }
          triangleCount++;
          triangleCount++;
        }
        if(i == (xP - 1) && !SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId1] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId1] = nodeCount;
            nodeCount++;
          }
          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId2] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId2] = nodeCount;
            nodeCount++;
          }
          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId3] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId3] = nodeCount;
            nodeCount++;
          }
          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId4] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId4] = nodeCount;
            nodeCount++;
          }
          triangleCount++;
          triangleCount++;
        }
        if(i < xP - 1 && featureIds[point] != featureIds[neigh1])
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId1] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId1] = nodeCount;
            nodeCount++;
          }
          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId2] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId2] = nodeCount;
            nodeCount++;
          }
          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId3] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId3] = nodeCount;
            nodeCount++;
          }
          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId4] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId4] = nodeCount;
            nodeCount++;
          }
          triangleCount++;
          triangleCount++;
        }
        if(j == (yP - 1) && !SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId1] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId1] = nodeCount;
            nodeCount++;
          }
          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          if(nodeIds[nodeId2] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId2] = nodeCount;
            nodeCount++;
          }
          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId3] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId3] = nodeCount;
            nodeCount++;
          }
          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          if(nodeIds[nodeId4] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId4] = nodeCount;
            nodeCount++;
          }
          triangleCount++;
          triangleCount++;
        }
        if(j < yP - 1 && featureIds[point] != featureIds[neigh2])
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId1] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId1] = nodeCount;
            nodeCount++;
          }
          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          if(nodeIds[nodeId2] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId2] = nodeCount;
            nodeCount++;
          }
          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId3] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId3] = nodeCount;
            nodeCount++;
          }
          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          if(nodeIds[nodeId4] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId4] = nodeCount;
            nodeCount++;
          }
          triangleCount++;
          triangleCount++;
        }
        if(k == (zP - 1) && !SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
        {
          nodeId1 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId1] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId1] = nodeCount;
            nodeCount++;
          }
          nodeId2 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          if(nodeIds[nodeId2] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId2] = nodeCount;
            nodeCount++;
          }
          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId3] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId3] = nodeCount;
            nodeCount++;
          }
          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          if(nodeIds[nodeId4] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId4] = nodeCount;
            nodeCount++;
          }
          triangleCount++;
          triangleCount++;
        }
        if(k < zP - 1 && featureIds[point] != featureIds[neigh3])
        {
          nodeId1 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId1] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId1] = nodeCount;
            nodeCount++;
          }
          nodeId2 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          if(nodeIds[nodeId2] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId2] = nodeCount;
            nodeCount++;
          }
          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId3] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId3] = nodeCount;
            nodeCount++;
          }
          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          if(nodeIds[nodeId4] == std::numeric_limits<usize>::max())
          {
            nodeIds[nodeId4] = nodeCount;
            nodeCount++;
          }
          triangleCount++;
          triangleCount++;
        }
      }
    }
  }
}

void QuickSurfaceMeshDirect::createNodesAndTriangles(std::vector<MeshIndexType>& m_NodeIds, MeshIndexType nodeCount, MeshIndexType triangleCount)
{
  if(m_ShouldCancel)
  {
    return;
  }
  m_MessageHandler(IFilter::Message::Type::Info, "Creating mesh");

  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  // Tuple-transfer feature allocation needs the greatest observed Feature ID.
  usize numFeatures = 0;
  usize numTuples = featureIds.getNumberOfTuples();
  for(usize i = 0; i < numTuples; i++)
  {
    const usize featureId = featureIds[i];
    if(featureId > numFeatures)
    {
      numFeatures = static_cast<usize>(featureId);
    }
  }

  auto* grid = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->GridGeomDataPath);

  SizeVec3 udims = grid->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];

  // Unique owners classify each node after every incident face is generated.
  std::vector<std::set<int32>> ownerLists;

  MeshIndexType point = 0;
  MeshIndexType neigh1 = 0;
  MeshIndexType neigh2 = 0;
  MeshIndexType neigh3 = 0;

  MeshIndexType nodeId1 = 0;
  MeshIndexType nodeId2 = 0;
  MeshIndexType nodeId3 = 0;
  MeshIndexType nodeId4 = 0;

  auto* triangleGeom = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  ShapeType tDims = {nodeCount};
  triangleGeom->resizeVertexList(nodeCount);
  triangleGeom->resizeFaceList(triangleCount);
  triangleGeom->getFaceAttributeMatrix()->resizeTuples({triangleCount});
  triangleGeom->getVertexAttributeMatrix()->resizeTuples(tDims);

  auto& faceLabelsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef();

  auto& nodeTypes = m_DataStructure.getDataAs<Int8Array>(m_InputValues->NodeTypesDataPath)->getDataStoreRef();
  nodeTypes.resizeTuples({nodeCount});

  VertexStore& vertex = triangleGeom->getVertices()->getDataStoreRef();
  TriStore& triangle = triangleGeom->getFaces()->getDataStoreRef();

  ownerLists.resize(nodeCount);

  // Build transfer functions once. Each emitted triangle receives its cell and
  // feature values after face labels establish source ordering.
  std::vector<std::shared_ptr<AbstractTupleTransfer>> tupleTransferFunctions;
  for(usize i = 0; i < m_InputValues->SelectedCellDataArrayPaths.size(); i++)
  {
    ::AddTupleTransferInstance(m_DataStructure, m_InputValues->SelectedCellDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i], tupleTransferFunctions);
  }

  for(usize i = 0; i < m_InputValues->SelectedFeatureDataArrayPaths.size(); i++)
  {
    ::AddFeatureTupleTransferInstance(m_DataStructure, m_InputValues->SelectedFeatureDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i + m_InputValues->SelectedCellDataArrayPaths.size()],
                                      m_InputValues->FeatureIdsArrayPath, tupleTransferFunctions);
  }

  MeshIndexType triangleIndex = 0;
  for(MeshIndexType k = 0; k < zP; k++)
  {
    // Check once per Z plane to keep the face loop free of atomic reads.
    if(m_ShouldCancel)
    {
      return;
    }
    for(MeshIndexType j = 0; j < yP; j++)
    {
      for(MeshIndexType i = 0; i < xP; i++)
      {

        point = (k * xP * yP) + (j * xP) + i;
        neigh1 = point + 1;
        neigh2 = point + xP;
        neigh3 = point + (xP * yP);

        if(i == 0 && !SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j + 1, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId2];
          faceLabelsStore[triangleIndex * 2] = -1;
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, point, point, faceLabelsStore);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId4];
          faceLabelsStore[triangleIndex * 2] = -1;
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, point, point, faceLabelsStore);
          }

          triangleIndex++;

          ownerLists[m_NodeIds[nodeId1]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId1]].insert(-1);
          ownerLists[m_NodeIds[nodeId2]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId2]].insert(-1);
          ownerLists[m_NodeIds[nodeId3]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId3]].insert(-1);
          ownerLists[m_NodeIds[nodeId4]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId4]].insert(-1);
        }
        if(j == 0 && !SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabelsStore[triangleIndex * 2] = -1;
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, point, point, faceLabelsStore);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId4];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabelsStore[triangleIndex * 2] = -1;
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, point, point, faceLabelsStore);
          }

          triangleIndex++;

          ownerLists[m_NodeIds[nodeId1]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId1]].insert(-1);
          ownerLists[m_NodeIds[nodeId2]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId2]].insert(-1);
          ownerLists[m_NodeIds[nodeId3]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId3]].insert(-1);
          ownerLists[m_NodeIds[nodeId4]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId4]].insert(-1);
        }
        if(k == 0 && !SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j + 1, k, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j + 1, k, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId2];
          faceLabelsStore[triangleIndex * 2] = -1;
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, point, point, faceLabelsStore);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId4];
          faceLabelsStore[triangleIndex * 2] = -1;
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, point, point, faceLabelsStore);
          }

          triangleIndex++;

          ownerLists[m_NodeIds[nodeId1]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId1]].insert(-1);
          ownerLists[m_NodeIds[nodeId2]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId2]].insert(-1);
          ownerLists[m_NodeIds[nodeId3]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId3]].insert(-1);
          ownerLists[m_NodeIds[nodeId4]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId4]].insert(-1);
        }
        if(i == (xP - 1) && !SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j + 1, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabelsStore[triangleIndex * 2] = -1;
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, point, point, faceLabelsStore);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId4];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabelsStore[triangleIndex * 2] = -1;
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, point, point, faceLabelsStore);
          }

          triangleIndex++;

          ownerLists[m_NodeIds[nodeId1]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId1]].insert(-1);
          ownerLists[m_NodeIds[nodeId2]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId2]].insert(-1);
          ownerLists[m_NodeIds[nodeId3]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId3]].insert(-1);
          ownerLists[m_NodeIds[nodeId4]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId4]].insert(-1);
        }
        if(i < xP - 1 && featureIds[point] != featureIds[neigh1])
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j + 1, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabelsStore[triangleIndex * 2] = featureIds[neigh1];
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          if(featureIds[point] < featureIds[neigh1])
          {
            triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
            triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId2];
            faceLabelsStore[triangleIndex * 2] = featureIds[point];
            faceLabelsStore[triangleIndex * 2 + 1] = featureIds[neigh1];
          }

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, neigh1, point, faceLabelsStore);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId4];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabelsStore[triangleIndex * 2] = featureIds[neigh1];
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];
          if(featureIds[point] < featureIds[neigh1])
          {
            triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
            triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId4];
            faceLabelsStore[triangleIndex * 2] = featureIds[point];
            faceLabelsStore[triangleIndex * 2 + 1] = featureIds[neigh1];
          }

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, neigh1, point, faceLabelsStore);
          }

          triangleIndex++;

          ownerLists[m_NodeIds[nodeId1]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId1]].insert(featureIds[neigh1]);
          ownerLists[m_NodeIds[nodeId2]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId2]].insert(featureIds[neigh1]);
          ownerLists[m_NodeIds[nodeId3]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId3]].insert(featureIds[neigh1]);
          ownerLists[m_NodeIds[nodeId4]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId4]].insert(featureIds[neigh1]);
        }
        if(j == (yP - 1) && !SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j + 1, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j + 1, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabelsStore[triangleIndex * 2] = -1;
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, point, point, faceLabelsStore);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId4];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabelsStore[triangleIndex * 2] = -1;
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, point, point, faceLabelsStore);
          }

          triangleIndex++;

          ownerLists[m_NodeIds[nodeId1]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId1]].insert(-1);
          ownerLists[m_NodeIds[nodeId2]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId2]].insert(-1);
          ownerLists[m_NodeIds[nodeId3]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId3]].insert(-1);
          ownerLists[m_NodeIds[nodeId4]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId4]].insert(-1);
        }
        if(j < yP - 1 && featureIds[point] != featureIds[neigh2])
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j + 1, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j + 1, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId2];
          faceLabelsStore[triangleIndex * 2] = featureIds[neigh2];
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];
          if(featureIds[point] < featureIds[neigh2])
          {
            triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId2];
            triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
            faceLabelsStore[triangleIndex * 2] = featureIds[point];
            faceLabelsStore[triangleIndex * 2 + 1] = featureIds[neigh2];
          }

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, neigh2, point, faceLabelsStore);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId4];
          faceLabelsStore[triangleIndex * 2] = featureIds[neigh2];
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];
          if(featureIds[point] < featureIds[neigh2])
          {
            triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId4];
            triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
            faceLabelsStore[triangleIndex * 2] = featureIds[point];
            faceLabelsStore[triangleIndex * 2 + 1] = featureIds[neigh2];
          }

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, neigh2, point, faceLabelsStore);
          }

          triangleIndex++;

          ownerLists[m_NodeIds[nodeId1]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId1]].insert(featureIds[neigh2]);
          ownerLists[m_NodeIds[nodeId2]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId2]].insert(featureIds[neigh2]);
          ownerLists[m_NodeIds[nodeId3]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId3]].insert(featureIds[neigh2]);
          ownerLists[m_NodeIds[nodeId4]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId4]].insert(featureIds[neigh2]);
        }
        if(k == (zP - 1) && !SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
        {
          nodeId1 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j, k + 1, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j, k + 1, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId2];
          faceLabelsStore[triangleIndex * 2] = -1;
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, point, point, faceLabelsStore);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId4];
          faceLabelsStore[triangleIndex * 2] = -1;
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, point, point, faceLabelsStore);
          }

          triangleIndex++;

          ownerLists[m_NodeIds[nodeId1]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId1]].insert(-1);
          ownerLists[m_NodeIds[nodeId2]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId2]].insert(-1);
          ownerLists[m_NodeIds[nodeId3]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId3]].insert(-1);
          ownerLists[m_NodeIds[nodeId4]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId4]].insert(-1);
        }
        if(k < zP - 1 && featureIds[point] != featureIds[neigh3])
        {
          nodeId1 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j, k + 1, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j, k + 1, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          ::GetGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          ::GetGridCoordinates(grid, i, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabelsStore[triangleIndex * 2] = featureIds[neigh3];
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];
          if(featureIds[point] < featureIds[neigh3])
          {
            triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
            triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId2];
            faceLabelsStore[triangleIndex * 2] = featureIds[point];
            faceLabelsStore[triangleIndex * 2 + 1] = featureIds[neigh3];
          }

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, neigh3, point, faceLabelsStore);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId4];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabelsStore[triangleIndex * 2] = featureIds[neigh3];
          faceLabelsStore[triangleIndex * 2 + 1] = featureIds[point];
          if(featureIds[point] < featureIds[neigh3])
          {
            triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
            triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId4];
            faceLabelsStore[triangleIndex * 2] = featureIds[point];
            faceLabelsStore[triangleIndex * 2 + 1] = featureIds[neigh3];
          }

          for(const auto& tupleTransferFunction : tupleTransferFunctions)
          {
            tupleTransferFunction->quickSurfaceTransfer(triangleIndex, neigh3, point, faceLabelsStore);
          }

          triangleIndex++;

          ownerLists[m_NodeIds[nodeId1]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId1]].insert(featureIds[neigh3]);
          ownerLists[m_NodeIds[nodeId2]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId2]].insert(featureIds[neigh3]);
          ownerLists[m_NodeIds[nodeId3]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId3]].insert(featureIds[neigh3]);
          ownerLists[m_NodeIds[nodeId4]].insert(featureIds[point]);
          ownerLists[m_NodeIds[nodeId4]].insert(featureIds[neigh3]);
        }
      }
    }
  }

  // Node type caps the distinct-owner count at four. The exterior owner (-1)
  // adds ten to the classification.
  Int8AbstractDataStore& m_NodeTypes = m_DataStructure.getDataAs<Int8Array>(m_InputValues->NodeTypesDataPath)->getDataStoreRef();

  for(usize i = 0; i < nodeCount; i++)
  {
    // Check each node because classification follows the full face-generation pass.
    if(m_ShouldCancel)
    {
      return;
    }

    auto& ownerList = ownerLists[i];

    m_NodeTypes[i] = static_cast<int8>(ownerList.size());
    if(m_NodeTypes[i] > 4)
    {
      m_NodeTypes[i] = 4;
    }
    if(ownerList.find(-1) != ownerList.end())
    {
      m_NodeTypes[i] += 10;
    }
  }
}
