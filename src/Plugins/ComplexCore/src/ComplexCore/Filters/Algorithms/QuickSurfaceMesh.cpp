#include "QuickSurfaceMesh.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include "TupleTransfer.hpp"

#include <array>
#include <random>
#include <unordered_map>

using namespace complex;

// -----------------------------------------------------------------------------
namespace
{
template <class T>
inline void hashCombine(size_t& seed, const T& obj)
{
  std::hash<T> hasher;
  seed ^= hasher(obj) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// -----------------------------------------------------------------------------
using Vertex = std::array<float, 3>;
using Edge = std::array<AbstractGeometry::MeshIndexType, 2>;

// -----------------------------------------------------------------------------
struct VertexHasher
{
  size_t operator()(const Vertex& vert) const
  {
    size_t hash = std::hash<float>()(vert[0]);
    hashCombine(hash, vert[1]);
    hashCombine(hash, vert[2]);
    return hash;
  }
};

// -----------------------------------------------------------------------------
struct EdgeHasher
{
  size_t operator()(const Edge& edge) const
  {
    size_t hash = std::hash<AbstractGeometry::MeshIndexType>()(edge[0]);
    hashCombine(hash, edge[1]);
    return hash;
  }
};

// -----------------------------------------------------------------------------
using VertexMap = std::unordered_map<Vertex, AbstractGeometry::MeshIndexType, VertexHasher>;
using EdgeMap = std::unordered_map<Edge, AbstractGeometry::MeshIndexType, EdgeHasher>;

} // namespace

// -----------------------------------------------------------------------------
QuickSurfaceMesh::QuickSurfaceMesh(DataStructure& dataStructure, QuickSurfaceMeshInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_Inputs(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
QuickSurfaceMesh::~QuickSurfaceMesh() noexcept = default;

// -----------------------------------------------------------------------------
Result<> QuickSurfaceMesh::operator()()
{
  DataObject::IdType parentGroupId = m_DataStructure.getId(m_Inputs->pParentDataGroupPath).value();
  // Get the ImageGeometry
  AbstractGeometryGrid& grid = m_DataStructure.getDataRefAs<AbstractGeometryGrid>(m_Inputs->pGridGeomDataPath);

  // Get the Created Triangle Geometry
  TriangleGeom& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_Inputs->pTriangleGeometryPath);

  SizeVec3 udims = grid.getDimensions();

  size_t xP = udims[0];
  size_t yP = udims[1];
  size_t zP = udims[2];

  std::vector<std::set<int32_t>> ownerLists;

  size_t possibleNumNodes = (xP + 1) * (yP + 1) * (zP + 1);
  std::vector<MeshIndexType> nodeIds(possibleNumNodes, std::numeric_limits<size_t>::max());

  MeshIndexType nodeCount = 0;
  MeshIndexType triangleCount = 0;

  if(m_Inputs->pFixProblemVoxels)
  {
    correctProblemVoxels();
  }

  determineActiveNodes(nodeIds, nodeCount, triangleCount);

  // now create node and triangle arrays knowing the number that will be needed
  triangleGeom.resizeFaceList(triangleCount);
  triangleGeom.resizeVertexList(nodeCount);

  // Resize the Face Arrays that are being copied over from the ImageGeom Cell Data
  std::vector<usize> tupleShape = {triangleCount};
  for(const auto& dataPath : m_Inputs->pCreatedDataArrayPaths)
  {
    Result<> result = complex::ResizeAndReplaceDataArray(m_DataStructure, dataPath, tupleShape, complex::IDataAction::Mode::Execute);
  }

  createNodesAndTriangles(nodeIds, nodeCount, triangleCount);

  if(m_Inputs->pGenerateTripleLines)
  {
    AbstractGeometry::SharedTriList* triangle = triangleGeom.getFaces();
    AbstractGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

    EdgeGeom* edgeGeom = EdgeGeom::Create(m_DataStructure, "[Edge Geometry]", parentGroupId);
    edgeGeom->setVertices(vertices);

    Int32Array& nodeTypes = m_DataStructure.getDataRefAs<Int32Array>(m_Inputs->pFaceLabelsDataPath);

    MeshIndexType edgeCount = 0;
    for(MeshIndexType i = 0; i < triangleCount; i++)
    {
      MeshIndexType n1 = (*triangle)[3 * i + 0];
      MeshIndexType n2 = (*triangle)[3 * i + 1];
      MeshIndexType n3 = (*triangle)[3 * i + 2];
      if(nodeTypes[n1] >= 3 && nodeTypes[n2] >= 3)
      {
        edgeCount++;
      }
      if(nodeTypes[n1] >= 3 && nodeTypes[n3] >= 3)
      {
        edgeCount++;
      }
      if(nodeTypes[n2] >= 3 && nodeTypes[n3] >= 3)
      {
        edgeCount++;
      }
    }

    std::string edgeGeometryName = "[Edge Geometry]";
    DataPath edgeGeometryDataPath = m_Inputs->pParentDataGroupPath.createChildPath(edgeGeometryName);
    std::string sharedEdgeListName = "SharedEdgeList";
    size_t numEdges = edgeCount;
    size_t numEdgeComps = 2;
    AbstractGeometry::SharedEdgeList* edges =
        AbstractGeometry::SharedEdgeList::CreateWithStore<DataStore<MeshIndexType>>(m_DataStructure, sharedEdgeListName, {numEdges}, {numEdgeComps}, m_DataStructure.getId(edgeGeometryDataPath));

    edgeCount = 0;
    for(MeshIndexType i = 0; i < triangleCount; i++)
    {
      MeshIndexType n1 = (*triangle)[3 * i + 0];
      MeshIndexType n2 = (*triangle)[3 * i + 1];
      MeshIndexType n3 = (*triangle)[3 * i + 2];
      if(nodeTypes[n1] >= 3 && nodeTypes[n2] >= 3)
      {
        (*edges)[2 * edgeCount] = n1;
        (*edges)[2 * edgeCount + 1] = n2;
        edgeCount++;
      }
      if(nodeTypes[n1] >= 3 && nodeTypes[n3] >= 3)
      {
        (*edges)[2 * edgeCount] = n1;
        (*edges)[2 * edgeCount + 1] = n3;
        edgeCount++;
      }
      if(nodeTypes[n2] >= 3 && nodeTypes[n3] >= 3)
      {
        (*edges)[2 * edgeCount] = n2;
        (*edges)[2 * edgeCount + 1] = n3;
        edgeCount++;
      }
    }

    // Now that we all of that out of the way, generate the triple lines
    generateTripleLines();
  }

  return {};
}

// -----------------------------------------------------------------------------
void QuickSurfaceMesh::getGridCoordinates(const AbstractGeometryGrid* grid, size_t x, size_t y, size_t z, AbstractGeometry::SharedVertexList& verts, AbstractGeometry::MeshIndexType nodeIndex)
{
  complex::Point3D<float64> tmpCoords = grid->getPlaneCoords(x, y, z);
  verts[nodeIndex] = static_cast<float>(tmpCoords[0]);
  verts[nodeIndex + 1] = static_cast<float>(tmpCoords[1]);
  verts[nodeIndex + 2] = static_cast<float>(tmpCoords[2]);
}

// -----------------------------------------------------------------------------
void QuickSurfaceMesh::flipProblemVoxelCase1(Int32Array& featureIds, MeshIndexType v1, MeshIndexType v2, MeshIndexType v3, MeshIndexType v4, MeshIndexType v5, MeshIndexType v6)
{
  double rangeMin = 0.0;
  double rangeMax = 1.0;
  std::random_device randomDevice;           // Will be used to obtain a seed for the random number engine
  std::mt19937_64 generator(randomDevice()); // Standard mersenne_twister_engine seeded with rd()
  std::mt19937_64::result_type seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  generator.seed(seed);
  std::uniform_real_distribution<> distribution(rangeMin, rangeMax);

  float val = static_cast<float>(distribution(generator)); // Random remaining position.

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
void QuickSurfaceMesh::flipProblemVoxelCase2(Int32Array& featureIds, MeshIndexType v1, MeshIndexType v2, MeshIndexType v3, MeshIndexType v4)
{
  double rangeMin = 0.0;
  double rangeMax = 1.0;
  std::random_device randomDevice;           // Will be used to obtain a seed for the random number engine
  std::mt19937_64 generator(randomDevice()); // Standard mersenne_twister_engine seeded with rd()
  std::mt19937_64::result_type seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  generator.seed(seed);
  std::uniform_real_distribution<> distribution(rangeMin, rangeMax);

  float val = static_cast<float>(distribution(generator)); // Random remaining position.

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
void QuickSurfaceMesh::flipProblemVoxelCase3(Int32Array& featureIds, MeshIndexType v1, MeshIndexType v2, MeshIndexType v3)
{
  double rangeMin = 0.0;
  double rangeMax = 1.0;
  std::random_device randomDevice;           // Will be used to obtain a seed for the random number engine
  std::mt19937_64 generator(randomDevice()); // Standard mersenne_twister_engine seeded with rd()
  std::mt19937_64::result_type seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  generator.seed(seed);
  std::uniform_real_distribution<> distribution(rangeMin, rangeMax);

  float val = static_cast<float>(distribution(generator)); // Random remaining position.

  if(val < 0.5f)
  {
    featureIds[v2] = featureIds[v1];
  }
  else
  {
    featureIds[v3] = featureIds[v1];
  }
}

// -----------------------------------------------------------------------------
void QuickSurfaceMesh::correctProblemVoxels()
{

  m_MessageHandler(IFilter::Message::Type::Info, "Correcting Problem Voxels");

  AbstractGeometryGrid* grid = m_DataStructure.getDataAs<AbstractGeometryGrid>(m_Inputs->pGridGeomDataPath);

  Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_Inputs->pFeatureIdsArrayPath);

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
            flipProblemVoxelCase1(featureIds, v1, v2, v3, v6, v7, v8);
            count++;
          }
          if(f2 == f7 && f2 != f1 && f2 != f3 && f2 != f4 && f2 != f5 && f2 != f6 && f2 != f8)
          {
            flipProblemVoxelCase1(featureIds, v2, v1, v4, v5, v8, v7);
            count++;
          }
          if(f3 == f6 && f3 != f1 && f3 != f2 && f3 != f4 && f3 != f5 && f3 != f7 && f3 != f8)
          {
            flipProblemVoxelCase1(featureIds, v3, v1, v4, v5, v8, v6);
            count++;
          }
          if(f4 == f5 && f4 != f1 && f4 != f2 && f4 != f3 && f4 != f6 && f4 != f7 && f4 != f8)
          {
            flipProblemVoxelCase1(featureIds, v4, v2, v3, v6, v7, v5);
            count++;
          }
          if(f1 == f6 && f1 != f2 && f1 != f5)
          {
            flipProblemVoxelCase2(featureIds, v1, v2, v5, v6);
            count++;
          }
          if(f2 == f5 && f2 != f1 && f2 != f6)
          {
            flipProblemVoxelCase2(featureIds, v2, v1, v6, v5);
            count++;
          }
          if(f3 == f8 && f3 != f4 && f3 != f7)
          {
            flipProblemVoxelCase2(featureIds, v3, v4, v7, v8);
            count++;
          }
          if(f4 == f7 && f4 != f3 && f4 != f8)
          {
            flipProblemVoxelCase2(featureIds, v4, v3, v8, v7);
            count++;
          }
          if(f1 == f7 && f1 != f3 && f1 != f5)
          {
            flipProblemVoxelCase2(featureIds, v1, v3, v5, v7);
            count++;
          }
          if(f3 == f5 && f3 != f1 && f3 != f7)
          {
            flipProblemVoxelCase2(featureIds, v3, v1, v7, v5);
            count++;
          }
          if(f2 == f8 && f2 != f4 && f2 != f6)
          {
            flipProblemVoxelCase2(featureIds, v2, v4, v6, v8);
            count++;
          }
          if(f4 == f6 && f4 != f2 && f4 != f8)
          {
            flipProblemVoxelCase2(featureIds, v4, v2, v8, v6);
            count++;
          }
          if(f1 == f4 && f1 != f2 && f1 != f3)
          {
            flipProblemVoxelCase2(featureIds, v1, v2, v3, v4);
            count++;
          }
          if(f2 == f3 && f2 != f1 && f2 != f4)
          {
            flipProblemVoxelCase2(featureIds, v2, v1, v4, v3);
            count++;
          }
          if(f5 == f8 && f5 != f6 && f5 != f7)
          {
            flipProblemVoxelCase2(featureIds, v5, v6, v7, v8);
            count++;
          }
          if(f6 == f7 && f6 != f5 && f6 != f8)
          {
            flipProblemVoxelCase2(featureIds, v6, v5, v8, v7);
            count++;
          }
          if(f2 == f3 && f2 == f4 && f2 == f5 && f2 == f6 && f2 == f7 && f2 != f1 && f2 != f8)
          {
            flipProblemVoxelCase3(featureIds, v2, v1, v8);
            count++;
          }
          if(f1 == f3 && f1 == f4 && f1 == f5 && f1 == f7 && f2 == f8 && f1 != f2 && f1 != f7)
          {
            flipProblemVoxelCase3(featureIds, v1, v2, v7);
            count++;
          }
          if(f1 == f2 && f1 == f4 && f1 == f5 && f1 == f7 && f1 == f8 && f1 != f3 && f1 != f6)
          {
            flipProblemVoxelCase3(featureIds, v1, v3, v6);
            count++;
          }
          if(f1 == f2 && f1 == f3 && f1 == f6 && f1 == f7 && f1 == f8 && f1 != f4 && f1 != f5)
          {
            flipProblemVoxelCase3(featureIds, v1, v4, v5);
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
void QuickSurfaceMesh::determineActiveNodes(std::vector<MeshIndexType>& nodeIds, MeshIndexType& nodeCount, MeshIndexType& triangleCount)
{
  m_MessageHandler(IFilter::Message::Type::Info, "Determining active Nodes");

  AbstractGeometryGrid* grid = m_DataStructure.getDataAs<AbstractGeometryGrid>(m_Inputs->pGridGeomDataPath);

  Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_Inputs->pFeatureIdsArrayPath);

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
        if(j == 0)
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
        if(k == 0)
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
        if(i == (xP - 1))
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
  m_MessageHandler(IFilter::Message::Type::Info, "Creating mesh");

  Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_Inputs->pFeatureIdsArrayPath);

  size_t numFeatures = 0;
  size_t numTuples = featureIds.getNumberOfTuples();
  for(size_t i = 0; i < numTuples; i++)
  {
    if(static_cast<size_t>(featureIds[i]) > numFeatures)
    {
      numFeatures = static_cast<size_t>(featureIds[i]);
    }
  }

  AbstractGeometryGrid* grid = m_DataStructure.getDataAs<AbstractGeometryGrid>(m_Inputs->pGridGeomDataPath);

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

  TriangleGeom* triangleGeom = m_DataStructure.getDataAs<TriangleGeom>(m_Inputs->pTriangleGeometryPath);
  LinkedGeometryData& linkedGeometryData = triangleGeom->getLinkedGeometryData();

  std::vector<size_t> tDims = {nodeCount};
  triangleGeom->resizeVertexList(nodeCount);
  triangleGeom->resizeFaceList(triangleCount);

  // Remove and then insert a properly sized Int32Array for the FaceLabels
  m_DataStructure.removeData(m_Inputs->pFaceLabelsDataPath);
  Result<> faceLabelResult = complex::CreateArray<int32_t>(m_DataStructure, {triangleCount}, {2}, m_Inputs->pFaceLabelsDataPath, IDataAction::Mode::Execute);
  Int32Array& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_Inputs->pFaceLabelsDataPath);
  linkedGeometryData.addFaceData(m_Inputs->pFaceLabelsDataPath);

  // Remove and then insert a properly sized int8 for the NodeTypes
  m_DataStructure.removeData(m_Inputs->pNodeTypesDataPath);
  Result<> nodeTypeResult = complex::CreateArray<int8_t>(m_DataStructure, {nodeCount}, {1}, m_Inputs->pNodeTypesDataPath, IDataAction::Mode::Execute);
  Int32Array& nodeTypes = m_DataStructure.getDataRefAs<Int32Array>(m_Inputs->pFaceLabelsDataPath);
  linkedGeometryData.addVertexData(m_Inputs->pFaceLabelsDataPath);

  AbstractGeometry::SharedVertexList& vertex = *(triangleGeom->getVertices());
  AbstractGeometry::SharedTriList& triangle = *(triangleGeom->getFaces());

  ownerLists.resize(nodeCount);

  // Create a vector of TupleTransferFunctions for each of the Triangle Face to Vertex Data Arrays
  std::vector<std::shared_ptr<AbstractTupleTransfer>> tupleTransferFunctions;
  for(size_t i = 0; i < m_Inputs->pSelectedDataArrayPaths.size(); i++)
  {
    // Associate these arrays with the Triangle Face Data.
    linkedGeometryData.addFaceData(m_Inputs->pSelectedDataArrayPaths[i]);
    ::AddTupleTransferInstance(m_DataStructure, m_Inputs->pSelectedDataArrayPaths[i], m_Inputs->pCreatedDataArrayPaths[i], tupleTransferFunctions);
  }

  // Cycle through again assigning coordinates to each node and assigning node numbers and feature labels to each triangle
  MeshIndexType triangleIndex = 0;
  for(MeshIndexType k = 0; k < zP; k++)
  {
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
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          getGridCoordinates(grid, i, j, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          getGridCoordinates(grid, i, j + 1, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          getGridCoordinates(grid, i, j, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          getGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId2];
          faceLabels[triangleIndex * 2] = -1;
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, point, point, true);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId4];
          faceLabels[triangleIndex * 2] = -1;
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, point, point, true);
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
        if(j == 0)
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          getGridCoordinates(grid, i, j, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          getGridCoordinates(grid, i, j, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabels[triangleIndex * 2] = -1;
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, point, point, true);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId4];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabels[triangleIndex * 2] = -1;
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, point, point, true);
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
        if(k == 0)
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          getGridCoordinates(grid, i, j, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          getGridCoordinates(grid, i, j + 1, k, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j + 1, k, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId2];
          faceLabels[triangleIndex * 2] = -1;
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, point, point, true);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId4];
          faceLabels[triangleIndex * 2] = -1;
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, point, point, true);
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
        if(i == (xP - 1)) // Takes care of the end of a Row...
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j + 1, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabels[triangleIndex * 2] = -1;
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, point, point, true);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId4];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabels[triangleIndex * 2] = -1;
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, point, point, true);
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
        else if(featureIds[point] != featureIds[neigh1])
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j + 1, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabels[triangleIndex * 2] = featureIds[neigh1];
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          if(featureIds[point] < featureIds[neigh1])
          {
            triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
            triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId2];
            faceLabels[triangleIndex * 2] = featureIds[point];
            faceLabels[triangleIndex * 2 + 1] = featureIds[neigh1];
          }

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, neigh1, point, true);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId4];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabels[triangleIndex * 2] = featureIds[neigh1];
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];
          if(featureIds[point] < featureIds[neigh1])
          {
            triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
            triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId4];
            faceLabels[triangleIndex * 2] = featureIds[point];
            faceLabels[triangleIndex * 2 + 1] = featureIds[neigh1];
          }

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, neigh1, point, true);
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
          nodeId1 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j + 1, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          getGridCoordinates(grid, i, j + 1, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          getGridCoordinates(grid, i, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabels[triangleIndex * 2] = -1;
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, point, point, true);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId4];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabels[triangleIndex * 2] = -1;
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, point, point, true);
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
        else if(featureIds[point] != featureIds[neigh2])
        {
          nodeId1 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j + 1, k, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = (k * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          getGridCoordinates(grid, i, j + 1, k, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          getGridCoordinates(grid, i, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId2];
          faceLabels[triangleIndex * 2] = featureIds[neigh2];
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];
          if(featureIds[point] < featureIds[neigh2])
          {
            triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId2];
            triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
            faceLabels[triangleIndex * 2] = featureIds[point];
            faceLabels[triangleIndex * 2 + 1] = featureIds[neigh2];
          }

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, neigh2, point, true);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId4];
          faceLabels[triangleIndex * 2] = featureIds[neigh2];
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];
          if(featureIds[point] < featureIds[neigh2])
          {
            triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId4];
            triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
            faceLabels[triangleIndex * 2] = featureIds[point];
            faceLabels[triangleIndex * 2 + 1] = featureIds[neigh2];
          }

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, neigh2, point, true);
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
          nodeId1 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j, k + 1, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          getGridCoordinates(grid, i, j, k + 1, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          getGridCoordinates(grid, i, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId2];
          faceLabels[triangleIndex * 2] = -1;
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, point, point, true);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId4];
          faceLabels[triangleIndex * 2] = -1;
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, point, point, true);
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
        else if(featureIds[point] != featureIds[neigh3])
        {
          nodeId1 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j, k + 1, vertex, (m_NodeIds[nodeId1] * 3));

          nodeId2 = ((k + 1) * (xP + 1) * (yP + 1)) + (j * (xP + 1)) + i;
          getGridCoordinates(grid, i, j, k + 1, vertex, (m_NodeIds[nodeId2] * 3));

          nodeId3 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + (i + 1);
          getGridCoordinates(grid, i + 1, j + 1, k + 1, vertex, (m_NodeIds[nodeId3] * 3));

          nodeId4 = ((k + 1) * (xP + 1) * (yP + 1)) + ((j + 1) * (xP + 1)) + i;
          getGridCoordinates(grid, i, j + 1, k + 1, vertex, (m_NodeIds[nodeId4] * 3));

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId1];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabels[triangleIndex * 2] = featureIds[neigh3];
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];
          if(featureIds[point] < featureIds[neigh3])
          {
            triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
            triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId2];
            faceLabels[triangleIndex * 2] = featureIds[point];
            faceLabels[triangleIndex * 2 + 1] = featureIds[neigh3];
          }

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, neigh3, point, true);
          }

          triangleIndex++;

          triangle[triangleIndex * 3 + 0] = m_NodeIds[nodeId2];
          triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId4];
          triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId3];
          faceLabels[triangleIndex * 2] = featureIds[neigh3];
          faceLabels[triangleIndex * 2 + 1] = featureIds[point];
          if(featureIds[point] < featureIds[neigh3])
          {
            triangle[triangleIndex * 3 + 1] = m_NodeIds[nodeId3];
            triangle[triangleIndex * 3 + 2] = m_NodeIds[nodeId4];
            faceLabels[triangleIndex * 2] = featureIds[point];
            faceLabels[triangleIndex * 2 + 1] = featureIds[neigh3];
          }

          for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
          {
            tupleTransferFunctions[dataVectorIndex]->transfer(triangleIndex, neigh3, point, true);
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

  Int8Array& m_NodeTypes = m_DataStructure.getDataRefAs<Int8Array>(m_Inputs->pNodeTypesDataPath);

  for(size_t i = 0; i < nodeCount; i++)
  {
    m_NodeTypes[i] = static_cast<int8_t>(ownerLists[i].size());
    if(m_NodeTypes[i] > 4)
    {
      m_NodeTypes[i] = 4;
    }
    if(ownerLists[i].find(-1) != ownerLists[i].end())
    {
      m_NodeTypes[i] += 10;
    }
  }
}

// -----------------------------------------------------------------------------
void QuickSurfaceMesh::generateTripleLines()
{
  /**
   * This is a bit of experimental code where we define a triple line as an edge
   * that shares voxels with at least 3 unique Feature Ids. This is different
   * than saying that an edge is part of a triple line if it's nodes are considered
   * sharing at least 3 unique voxels. This code is not complete as it will only
   * find "interior" triple lines and no lines on the surface. I am going to leave
   * this bit of code in place for historical reasons so that we can refer to it
   * later if needed.
   * Mike Jackson, JULY 2018
   */
  m_MessageHandler(IFilter::Message::Type::Info, "Generating Triple Lines");

  //  DataContainer* m = getDataContainerArray()->getDataContainer(m_FeatureIdsArrayPath.getDataContainerName());
  //  DataContainer* sm = getDataContainerArray()->getDataContainer(getSurfaceDataContainerName());
  //
  //  AttributeMatrix* featAttrMat = sm->getAttributeMatrix(m_FeatureAttributeMatrixName);
  Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_Inputs->pFeatureIdsArrayPath);

  int32_t numFeatures = 0;
  size_t numTuples = featureIds.getNumberOfTuples();
  for(size_t i = 0; i < numTuples; i++)
  {
    if(featureIds[i] > numFeatures)
    {
      numFeatures = featureIds[i];
    }
  }

  std::vector<size_t> featDims = {static_cast<size_t>(numFeatures) + 1};

  ImageGeom* imageGeom = m_DataStructure.getDataAs<ImageGeom>(m_Inputs->pGridGeomDataPath);

  SizeVec3 udims = imageGeom->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];
  MeshIndexType point = 0, neigh1 = 0, neigh2 = 0, neigh3 = 0;

  std::set<int32_t> uFeatures;

  FloatVec3 origin = imageGeom->getOrigin();
  FloatVec3 res = imageGeom->getSpacing();

  VertexMap vertexMap;
  EdgeMap edgeMap;
  MeshIndexType vertCounter = 0;
  MeshIndexType edgeCounter = 0;

  // Cycle through again assigning coordinates to each node and assigning node numbers and feature labels to each triangle
  // int64_t triangleIndex = 0;
  for(size_t k = 0; k < zP - 1; k++)
  {
    for(size_t j = 0; j < yP - 1; j++)
    {
      for(size_t i = 0; i < xP - 1; i++)
      {

        point = (k * xP * yP) + (j * xP) + i;
        // Case 1
        neigh1 = point + 1;
        neigh2 = point + (xP * yP) + 1;
        neigh3 = point + (xP * yP);

        Vertex p0 = {{origin[0] + static_cast<float>(i) * res[0] + res[0], origin[1] + static_cast<float>(j) * res[1] + res[1], origin[2] + static_cast<float>(k) * res[2] + res[2]}};

        Vertex p1 = {{origin[0] + static_cast<float>(i) * res[0] + res[0], origin[1] + static_cast<float>(j) * res[1], origin[2] + static_cast<float>(k) * res[2] + res[2]}};

        Vertex p2 = {{origin[0] + static_cast<float>(i) * res[0], origin[1] + static_cast<float>(j) * res[1] + res[1], origin[2] + static_cast<float>(k) * res[2] + res[2]}};

        Vertex p3 = {{origin[0] + static_cast<float>(i) * res[0] + res[0], origin[1] + static_cast<float>(j) * res[1] + res[1], origin[2] + static_cast<float>(k) * res[2]}};

        uFeatures.clear();
        uFeatures.insert(featureIds[point]);
        uFeatures.insert(featureIds[neigh1]);
        uFeatures.insert(featureIds[neigh2]);
        uFeatures.insert(featureIds[neigh3]);

        if(uFeatures.size() > 2)
        {
          auto iter = vertexMap.find(p0);
          if(iter == vertexMap.end())
          {
            vertexMap[p0] = vertCounter++;
          }
          iter = vertexMap.find(p1);
          if(iter == vertexMap.end())
          {
            vertexMap[p1] = vertCounter++;
          }
          MeshIndexType i0 = vertexMap[p0];
          MeshIndexType i1 = vertexMap[p1];

          Edge tmpEdge = {{i0, i1}};
          auto eiter = edgeMap.find(tmpEdge);
          if(eiter == edgeMap.end())
          {
            edgeMap[tmpEdge] = edgeCounter++;
          }
        }

        // Case 2
        neigh1 = point + xP;
        neigh2 = point + (xP * yP) + xP;
        neigh3 = point + (xP * yP);

        uFeatures.clear();
        uFeatures.insert(featureIds[point]);
        uFeatures.insert(featureIds[neigh1]);
        uFeatures.insert(featureIds[neigh2]);
        uFeatures.insert(featureIds[neigh3]);
        if(uFeatures.size() > 2)
        {
          auto iter = vertexMap.find(p0);
          if(iter == vertexMap.end())
          {
            vertexMap[p0] = vertCounter++;
          }
          iter = vertexMap.find(p2);
          if(iter == vertexMap.end())
          {
            vertexMap[p2] = vertCounter++;
          }

          MeshIndexType i0 = vertexMap[p0];
          MeshIndexType i2 = vertexMap[p2];

          Edge tmpEdge = {{i0, i2}};
          auto eiter = edgeMap.find(tmpEdge);
          if(eiter == edgeMap.end())
          {
            edgeMap[tmpEdge] = edgeCounter++;
          }
        }

        // Case 3
        neigh1 = point + 1;
        neigh2 = point + xP + 1;
        neigh3 = point + +xP;

        uFeatures.clear();
        uFeatures.insert(featureIds[point]);
        uFeatures.insert(featureIds[neigh1]);
        uFeatures.insert(featureIds[neigh2]);
        uFeatures.insert(featureIds[neigh3]);
        if(uFeatures.size() > 2)
        {
          auto iter = vertexMap.find(p0);
          if(iter == vertexMap.end())
          {
            vertexMap[p0] = vertCounter++;
          }
          iter = vertexMap.find(p3);
          if(iter == vertexMap.end())
          {
            vertexMap[p3] = vertCounter++;
          }

          MeshIndexType i0 = vertexMap[p0];
          MeshIndexType i3 = vertexMap[p3];

          Edge tmpEdge = {{i0, i3}};
          auto eiter = edgeMap.find(tmpEdge);
          if(eiter == edgeMap.end())
          {
            edgeMap[tmpEdge] = edgeCounter++;
          }
        }
      }
    }
  }

  std::string edgeGeometryName = "[Edge Geometry]";
  DataPath edgeGeometryDataPath = m_Inputs->pParentDataGroupPath.createChildPath(edgeGeometryName);
  std::string sharedVertListName = "SharedVertList";
  DataPath sharedVertListDataPath = edgeGeometryDataPath.createChildPath(sharedVertListName);

  EdgeGeom* tripleLineEdge = EdgeGeom::Create(m_DataStructure, edgeGeometryName, m_DataStructure.getId(m_Inputs->pParentDataGroupPath));
  size_t numVerts = vertexMap.size();
  size_t numComps = 3;
  AbstractGeometry::SharedVertexList* vertices =
      Float32Array::CreateWithStore<DataStore<float>>(m_DataStructure, sharedVertListName, {numVerts}, {numComps}, m_DataStructure.getId(edgeGeometryDataPath));

  for(auto vert : vertexMap)
  {
    float v0 = vert.first[0];
    float v1 = vert.first[1];
    float v2 = vert.first[2];
    MeshIndexType idx = vert.second;
    vertices->getDataStore()->setValue(idx * numComps + 0, v0);
    vertices->getDataStore()->setValue(idx * numComps + 1, v1);
    vertices->getDataStore()->setValue(idx * numComps + 2, v2);
  }
  tripleLineEdge->setVertices(vertices);

  std::string sharedEdgeListName = "SharedEdgeList";
  size_t numEdges = edgeMap.size();
  size_t numEdgeComps = 2;
  AbstractGeometry::SharedEdgeList* edges =
      AbstractGeometry::SharedEdgeList::CreateWithStore<DataStore<MeshIndexType>>(m_DataStructure, sharedEdgeListName, {numEdges}, {numEdgeComps}, m_DataStructure.getId(edgeGeometryDataPath));

  for(auto edge : edgeMap)
  {
    MeshIndexType i0 = edge.first[0];
    MeshIndexType i1 = edge.first[1];
    MeshIndexType idx = edge.second;
    edges->getDataStore()->setValue(idx * numComps + 0, i0);
    edges->getDataStore()->setValue(idx * numComps + 1, i1);
  }
  tripleLineEdge->setEdges(edges);
}
