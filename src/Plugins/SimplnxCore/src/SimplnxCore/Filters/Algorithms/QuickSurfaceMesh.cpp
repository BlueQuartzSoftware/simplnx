#include "QuickSurfaceMesh.hpp"
#include "TupleTransfer.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
namespace
{
constexpr double k_RangeMin = 0.0;
constexpr double k_RangeMax = 1.0;
std::random_device k_RandomDevice;             // Will be used to obtain a seed for the random number engine
std::mt19937_64 k_Generator(k_RandomDevice()); // Standard mersenne_twister_engine seeded with rd()
std::mt19937_64::result_type k_Seed = 3412341234123412;
std::uniform_real_distribution<> k_Distribution(k_RangeMin, k_RangeMax);

// -----------------------------------------------------------------------------
void GetGridCoordinates(const IGridGeometry* grid, size_t x, size_t y, size_t z, QuickSurfaceMesh::VertexStore& verts, IGeometry::MeshIndexType nodeIndex)
{
  nx::core::Point3D<float64> tmpCoords = grid->getPlaneCoords(x, y, z);
  verts[nodeIndex] = static_cast<QuickSurfaceMesh::VertexStore::value_type>(tmpCoords[0]);
  verts[nodeIndex + 1] = static_cast<QuickSurfaceMesh::VertexStore::value_type>(tmpCoords[1]);
  verts[nodeIndex + 2] = static_cast<QuickSurfaceMesh::VertexStore::value_type>(tmpCoords[2]);
}

/**
 * @brief True when a bounding-box wall face backed by this Feature Id must be skipped.
 * A wall face always gets Face Labels {-1, featureId}, so the {-1, 0} rule reduces to
 * "the voxel behind the wall is background". Both the counting pass and the emit pass
 * MUST call this same function -- if they disagree, node ids go unassigned and
 * ownerLists is indexed out of bounds.
 * Takes the store and index (rather than the already-read Feature Id) so the mode check
 * short-circuits before touching the DataStore on the default (mode == Off) path, which the
 * counting pass otherwise runs for every boundary voxel even when the option is disabled.
 */
inline bool SkipWallFace(ChoicesParameter::ValueType mode, const Int32AbstractDataStore& featureIds, usize point)
{
  return mode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && featureIds[point] == 0;
}

// -----------------------------------------------------------------------------
void FlipProblemVoxelCase1(Int32AbstractDataStore& featureIds, QuickSurfaceMesh::MeshIndexType v1, QuickSurfaceMesh::MeshIndexType v2, QuickSurfaceMesh::MeshIndexType v3,
                           QuickSurfaceMesh::MeshIndexType v4, QuickSurfaceMesh::MeshIndexType v5, QuickSurfaceMesh::MeshIndexType v6)
{
  auto val = static_cast<float>(k_Distribution(k_Generator)); // Random remaining position.

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

// -----------------------------------------------------------------------------
void FlipProblemVoxelCase2(Int32AbstractDataStore& featureIds, QuickSurfaceMesh::MeshIndexType v1, QuickSurfaceMesh::MeshIndexType v2, QuickSurfaceMesh::MeshIndexType v3,
                           QuickSurfaceMesh::MeshIndexType v4)
{
  auto val = static_cast<float>(k_Distribution(k_Generator)); // Random remaining position.

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

// -----------------------------------------------------------------------------
void FlipProblemVoxelCase3(Int32AbstractDataStore& featureIds, QuickSurfaceMesh::MeshIndexType v1, QuickSurfaceMesh::MeshIndexType v2, QuickSurfaceMesh::MeshIndexType v3)
{
  auto val = static_cast<float>(k_Distribution(k_Generator)); // Random remaining position.

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

// -----------------------------------------------------------------------------
QuickSurfaceMesh::QuickSurfaceMesh(DataStructure& dataStructure, QuickSurfaceMeshInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
  k_Generator.seed(k_Seed);
}

// -----------------------------------------------------------------------------
QuickSurfaceMesh::~QuickSurfaceMesh() noexcept = default;

// -----------------------------------------------------------------------------
Result<> QuickSurfaceMesh::operator()()
{
  // Reject Feature Ids that collide with this algorithm's hard-coded exterior Face Label (-1) and
  // owner-set marker: a negative Feature Id would be silently misinterpreted downstream. This is a
  // mitigation for the underlying sentinel-collision design, not a fix -- see simplnx#1705. Run here
  // (execute), not preflight: a full-volume scan is too expensive to repeat on every GUI parameter edit.
  {
    const auto& featureIdsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
    Result<> sentinelCheck = MeshingUtilities::ValidateFeatureIdsAgainstSentinels(featureIdsStore, m_InputValues->FeatureIdsArrayPath, /*rejectMaxInt32=*/false, m_ShouldCancel, m_MessageHandler);
    if(sentinelCheck.invalid())
    {
      return sentinelCheck;
    }
  }

  // Get the ImageGeometry
  auto& grid = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->GridGeomDataPath);

  // Get the Created Triangle Geometry
  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  SizeVec3 udims = grid.getDimensions();

  size_t xP = udims[0];
  size_t yP = udims[1];
  size_t zP = udims[2];

  std::vector<std::set<int32_t>> ownerLists;

  size_t possibleNumNodes = (xP + 1) * (yP + 1) * (zP + 1);
  std::vector<MeshIndexType> nodeIds(possibleNumNodes, std::numeric_limits<size_t>::max());

  MeshIndexType nodeCount = 0;
  MeshIndexType triangleCount = 0;
  MeshIndexType suppressedFaceCount = 0;

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

  // now create node and triangle arrays knowing the number that will be needed
  ShapeType tupleShape = {triangleCount};
  triangleGeom.resizeFaceList(triangleCount);
  triangleGeom.resizeVertexList(nodeCount);
  triangleGeom.getFaceAttributeMatrix()->resizeTuples(tupleShape);
  triangleGeom.getVertexAttributeMatrix()->resizeTuples({nodeCount});

  // Resize the Face Arrays that are being copied over from the ImageGeom Cell Data
  for(const auto& dataPath : m_InputValues->CreatedDataArrayPaths)
  {
    Result<> result = nx::core::ResizeAndReplaceDataArray(m_DataStructure, dataPath, tupleShape, nx::core::IDataAction::Mode::Execute);
  }

  createNodesAndTriangles(nodeIds, nodeCount, triangleCount);
  if(m_ShouldCancel)
  {
    return {};
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
      return MakeErrorResult(-56341, fmt::format("Unable to generate the connectivity list for {} geometry.", triangleGeom.getName()));
    }
    const auto& connectivity = m_DataStructure.getDataRefAs<IGeometry::ElementDynamicList>(optionalId.value());

    m_MessageHandler("Repairing Windings...");
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

// -----------------------------------------------------------------------------
void QuickSurfaceMesh::correctProblemVoxels()
{
  m_MessageHandler(IFilter::Message::Type::Info, "Correcting Problem Voxels");

  auto* grid = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  SizeVec3 udims = grid->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];

  MeshIndexType v1 = 0, v2 = 0, v3 = 0, v4 = 0;
  MeshIndexType v5 = 0, v6 = 0, v7 = 0, v8 = 0;

  int32_t f1 = 0, f2 = 0, f3 = 0, f4 = 0;
  int32_t f5 = 0, f6 = 0, f7 = 0, f8 = 0;

  MeshIndexType row1 = 0, row2 = 0;
  MeshIndexType plane1 = 0, plane2 = 0;

  MeshIndexType count = 1;
  MeshIndexType iter = 0;
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
void QuickSurfaceMesh::determineActiveNodes(std::vector<MeshIndexType>& nodeIds, MeshIndexType& nodeCount, MeshIndexType& triangleCount, MeshIndexType& suppressedFaceCount)
{
  m_MessageHandler(IFilter::Message::Type::Info, "Determining active Nodes");

  auto* grid = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  Int32AbstractDataStore& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  SizeVec3 udims = grid->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];

  std::vector<std::set<int32_t>> ownerLists;

  MeshIndexType point = 0, neigh1 = 0, neigh2 = 0, neigh3 = 0;

  MeshIndexType nodeId1 = 0, nodeId2 = 0, nodeId3 = 0, nodeId4 = 0;

  // first determining which nodes are actually boundary nodes and
  // count number of nodes and triangles that will be created
  for(MeshIndexType k = 0; k < zP; k++)
  {
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

        if(i == 0)
        {
          if(::SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
          {
            suppressedFaceCount++;
          }
          else
          {
            nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
            if(nodeIds[nodeId1] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId1] = nodeCount;
              nodeCount++;
            }
            nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
            if(nodeIds[nodeId2] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId2] = nodeCount;
              nodeCount++;
            }
            nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
            if(nodeIds[nodeId3] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId3] = nodeCount;
              nodeCount++;
            }
            nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
            if(nodeIds[nodeId4] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId4] = nodeCount;
              nodeCount++;
            }
            triangleCount++;
            triangleCount++;
          }
        }
        if(j == 0)
        {
          if(::SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
          {
            suppressedFaceCount++;
          }
          else
          {
            nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
            if(nodeIds[nodeId1] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId1] = nodeCount;
              nodeCount++;
            }
            nodeId2 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
            if(nodeIds[nodeId2] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId2] = nodeCount;
              nodeCount++;
            }
            nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
            if(nodeIds[nodeId3] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId3] = nodeCount;
              nodeCount++;
            }
            nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
            if(nodeIds[nodeId4] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId4] = nodeCount;
              nodeCount++;
            }
            triangleCount++;
            triangleCount++;
          }
        }
        if(k == 0)
        {
          if(::SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
          {
            suppressedFaceCount++;
          }
          else
          {
            nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
            if(nodeIds[nodeId1] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId1] = nodeCount;
              nodeCount++;
            }
            nodeId2 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
            if(nodeIds[nodeId2] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId2] = nodeCount;
              nodeCount++;
            }
            nodeId3 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
            if(nodeIds[nodeId3] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId3] = nodeCount;
              nodeCount++;
            }
            nodeId4 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
            if(nodeIds[nodeId4] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId4] = nodeCount;
              nodeCount++;
            }
            triangleCount++;
            triangleCount++;
          }
        }
        if(i == (xP - 1))
        {
          if(::SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
          {
            suppressedFaceCount++;
          }
          else
          {
            nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
            if(nodeIds[nodeId1] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId1] = nodeCount;
              nodeCount++;
            }
            nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
            if(nodeIds[nodeId2] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId2] = nodeCount;
              nodeCount++;
            }
            nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
            if(nodeIds[nodeId3] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId3] = nodeCount;
              nodeCount++;
            }
            nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
            if(nodeIds[nodeId4] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId4] = nodeCount;
              nodeCount++;
            }
            triangleCount++;
            triangleCount++;
          }
        }
        else if(featureIds[point] != featureIds[neigh1])
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId1] == std::numeric_limits<size_t>::max())
          {
            nodeIds[nodeId1] = nodeCount;
            nodeCount++;
          }
          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId2] == std::numeric_limits<size_t>::max())
          {
            nodeIds[nodeId2] = nodeCount;
            nodeCount++;
          }
          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId3] == std::numeric_limits<size_t>::max())
          {
            nodeIds[nodeId3] = nodeCount;
            nodeCount++;
          }
          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId4] == std::numeric_limits<size_t>::max())
          {
            nodeIds[nodeId4] = nodeCount;
            nodeCount++;
          }
          triangleCount++;
          triangleCount++;
        }
        if(j == (yP - 1))
        {
          if(::SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
          {
            suppressedFaceCount++;
          }
          else
          {
            nodeId1 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
            if(nodeIds[nodeId1] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId1] = nodeCount;
              nodeCount++;
            }
            nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
            if(nodeIds[nodeId2] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId2] = nodeCount;
              nodeCount++;
            }
            nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
            if(nodeIds[nodeId3] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId3] = nodeCount;
              nodeCount++;
            }
            nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
            if(nodeIds[nodeId4] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId4] = nodeCount;
              nodeCount++;
            }
            triangleCount++;
            triangleCount++;
          }
        }
        else if(featureIds[point] != featureIds[neigh2])
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId1] == std::numeric_limits<size_t>::max())
          {
            nodeIds[nodeId1] = nodeCount;
            nodeCount++;
          }
          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          if(nodeIds[nodeId2] == std::numeric_limits<size_t>::max())
          {
            nodeIds[nodeId2] = nodeCount;
            nodeCount++;
          }
          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId3] == std::numeric_limits<size_t>::max())
          {
            nodeIds[nodeId3] = nodeCount;
            nodeCount++;
          }
          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          if(nodeIds[nodeId4] == std::numeric_limits<size_t>::max())
          {
            nodeIds[nodeId4] = nodeCount;
            nodeCount++;
          }
          triangleCount++;
          triangleCount++;
        }
        if(k == (zP - 1))
        {
          if(::SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
          {
            suppressedFaceCount++;
          }
          else
          {
            nodeId1 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
            if(nodeIds[nodeId1] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId1] = nodeCount;
              nodeCount++;
            }
            nodeId2 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
            if(nodeIds[nodeId2] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId2] = nodeCount;
              nodeCount++;
            }
            nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
            if(nodeIds[nodeId3] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId3] = nodeCount;
              nodeCount++;
            }
            nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
            if(nodeIds[nodeId4] == std::numeric_limits<size_t>::max())
            {
              nodeIds[nodeId4] = nodeCount;
              nodeCount++;
            }
            triangleCount++;
            triangleCount++;
          }
        }
        else if(k < zP - 1 && featureIds[point] != featureIds[neigh3])
        {
          nodeId1 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId1] == std::numeric_limits<size_t>::max())
          {
            nodeIds[nodeId1] = nodeCount;
            nodeCount++;
          }
          nodeId2 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          if(nodeIds[nodeId2] == std::numeric_limits<size_t>::max())
          {
            nodeIds[nodeId2] = nodeCount;
            nodeCount++;
          }
          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          if(nodeIds[nodeId3] == std::numeric_limits<size_t>::max())
          {
            nodeIds[nodeId3] = nodeCount;
            nodeCount++;
          }
          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          if(nodeIds[nodeId4] == std::numeric_limits<size_t>::max())
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

// -----------------------------------------------------------------------------
void QuickSurfaceMesh::createNodesAndTriangles(std::vector<MeshIndexType>& m_NodeIds, MeshIndexType nodeCount, MeshIndexType triangleCount)
{
  if(m_ShouldCancel)
  {
    return;
  }
  m_MessageHandler(IFilter::Message::Type::Info, "Creating mesh");

  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  size_t numFeatures = 0;
  size_t numTuples = featureIds.getNumberOfTuples();
  for(size_t i = 0; i < numTuples; i++)
  {
    const size_t featureId = featureIds[i];
    if(featureId > numFeatures)
    {
      numFeatures = static_cast<size_t>(featureId);
    }
  }

  auto* grid = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->GridGeomDataPath);

  SizeVec3 udims = grid->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];

  std::vector<std::set<int32_t>> ownerLists;

  MeshIndexType point = 0;
  MeshIndexType neigh1 = 0;
  MeshIndexType neigh2 = 0;
  MeshIndexType neigh3 = 0;

  MeshIndexType nodeId1 = 0;
  MeshIndexType nodeId2 = 0;
  MeshIndexType nodeId3 = 0;
  MeshIndexType nodeId4 = 0;

  auto* triangleGeom = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  std::vector<size_t> tDims = {nodeCount};
  triangleGeom->resizeVertexList(nodeCount);
  triangleGeom->resizeFaceList(triangleCount);
  triangleGeom->getFaceAttributeMatrix()->resizeTuples({triangleCount});
  triangleGeom->getVertexAttributeMatrix()->resizeTuples(tDims);

  auto& faceLabelsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef();

  // Resize the NodeTypes array
  auto& nodeTypes = m_DataStructure.getDataAs<Int8Array>(m_InputValues->NodeTypesDataPath)->getDataStoreRef();
  nodeTypes.resizeTuples({nodeCount});

  QuickSurfaceMesh::VertexStore& vertex = triangleGeom->getVertices()->getDataStoreRef();
  QuickSurfaceMesh::TriStore& triangle = triangleGeom->getFaces()->getDataStoreRef();

  ownerLists.resize(nodeCount);

  // Create a vector of TupleTransferFunctions for each of the Triangle Face to VertexType Data Arrays
  std::vector<std::shared_ptr<AbstractTupleTransfer>> tupleTransferFunctions;
  for(size_t i = 0; i < m_InputValues->SelectedCellDataArrayPaths.size(); i++)
  {
    // Associate these arrays with the Triangle Face Data.
    ::AddTupleTransferInstance(m_DataStructure, m_InputValues->SelectedCellDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i], tupleTransferFunctions);
  }

  for(size_t i = 0; i < m_InputValues->SelectedFeatureDataArrayPaths.size(); i++)
  {
    // Associate these arrays with the Triangle Face Data.
    ::AddFeatureTupleTransferInstance(m_DataStructure, m_InputValues->SelectedFeatureDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i + m_InputValues->SelectedCellDataArrayPaths.size()],
                                      m_InputValues->FeatureIdsArrayPath, tupleTransferFunctions);
  }

  // Cycle through again assigning coordinates to each node and assigning node numbers and feature labels to each triangle
  MeshIndexType triangleIndex = 0;
  for(MeshIndexType k = 0; k < zP; k++)
  {
    if(m_ShouldCancel)
    {
      return;
    }
    for(MeshIndexType j = 0; j < yP; j++)
    {
      for(MeshIndexType i = 0; i < xP; i++)
      {

        point = (k * xP * yP) + (j * xP) + i;
        neigh1 = point + 1; // <== What happens if we are at the end of a row?
        neigh2 = point + xP;
        neigh3 = point + (xP * yP);

        if(i == 0)
        {
          if(!::SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
          {
            nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
            ::GetGridCoordinates(grid, i, j, k, vertex, (m_NodeIds[nodeId1] * 3));

            nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
            ::GetGridCoordinates(grid, i, j + 1, k, vertex, (m_NodeIds[nodeId2] * 3));

            nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
            ::GetGridCoordinates(grid, i, j, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

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
        }
        if(j == 0)
        {
          if(!::SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
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
        }
        if(k == 0)
        {
          if(!::SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
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
        }
        if(i == (xP - 1)) // Takes care of the end of a Row...
        {
          if(!::SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
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
        }
        else if(featureIds[point] != featureIds[neigh1])
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
        if(j == (yP - 1)) // Takes care of the end of a column
        {
          if(!::SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
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
        }
        else if(featureIds[point] != featureIds[neigh2])
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
        if(k == (zP - 1)) // Takes care of the end of a Pillar
        {
          if(!::SkipWallFace(m_InputValues->BoundingBoxSkinMode, featureIds, point))
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
        }
        else if(featureIds[point] != featureIds[neigh3])
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

  Int8AbstractDataStore& m_NodeTypes = m_DataStructure.getDataAs<Int8Array>(m_InputValues->NodeTypesDataPath)->getDataStoreRef();

  for(size_t i = 0; i < nodeCount; i++)
  {
    auto& ownerList = ownerLists[i];

    m_NodeTypes[i] = static_cast<int8_t>(ownerList.size());
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
