#include "GeometryUtilities.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <utility>

using namespace nx::core;

namespace
{
constexpr uint64 k_FullRange = 0;
constexpr uint64 k_UserDefinedRange = 1;
constexpr float32 k_PartitionEdgePadding = 0.000001;
const FloatVec3 k_Padding(k_PartitionEdgePadding, k_PartitionEdgePadding, k_PartitionEdgePadding);

} // namespace

GeometryUtilities::FindUniqueIdsImpl::FindUniqueIdsImpl(VertexStore& vertexStore, const std::vector<std::vector<size_t>>& nodesInBin, nx::core::Int64AbstractDataStore& uniqueIds)
: m_VertexStore(vertexStore)
, m_NodesInBin(nodesInBin)
, m_UniqueIds(uniqueIds)
{
}

// -----------------------------------------------------------------------------
void GeometryUtilities::FindUniqueIdsImpl::convert(size_t start, size_t end) const
{
  for(size_t i = start; i < end; i++)
  {
    for(size_t j = 0; j < m_NodesInBin[i].size(); j++)
    {
      size_t node1 = m_NodesInBin[i][j];
      if(m_UniqueIds[node1] == static_cast<int64_t>(node1))
      {
        for(size_t k = j + 1; k < m_NodesInBin[i].size(); k++)
        {
          size_t node2 = m_NodesInBin[i][k];
          if(m_VertexStore[node1 * 3] == m_VertexStore[node2 * 3] && m_VertexStore[node1 * 3 + 1] == m_VertexStore[node2 * 3 + 1] && m_VertexStore[node1 * 3 + 2] == m_VertexStore[node2 * 3 + 2])
          {
            m_UniqueIds[node2] = node1;
          }
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------
void GeometryUtilities::FindUniqueIdsImpl::operator()(const Range& range) const
{
  convert(range.min(), range.max());
}

BoundingBox3Df GeometryUtilities::GetPaddedNodePartitionBounds(const INodeGeometry0D& geometry)
{
  const auto bounds = geometry.getBoundingBox();
  if(!bounds.isValid())
  {
    return bounds;
  }
  return {bounds.getMinPoint() - k_Padding, bounds.getMaxPoint() + k_Padding};
}

Result<FloatVec3> GeometryUtilities::CalculatePartitionLengthsByPartitionCount(const INodeGeometry0D& geometry, const SizeVec3& numberOfPartitionsPerAxis)
{
  const auto boundingBox = GetPaddedNodePartitionBounds(geometry);
  if(!boundingBox.isValid())
  {
    return {};
  }
  return GeometryUtilities::CalculatePartitionLengthsOfBoundingBox(boundingBox, numberOfPartitionsPerAxis);
}

Result<FloatVec3> GeometryUtilities::CalculatePartitionLengthsByPartitionCount(const ImageGeom& geometry, const SizeVec3& numberOfPartitionsPerAxis)
{
  SizeVec3 dims = geometry.getDimensions();
  FloatVec3 spacing = geometry.getSpacing();
  float32 lengthX = static_cast<float32>(dims.getX()) / static_cast<float32>(numberOfPartitionsPerAxis.getX()) * spacing[0];
  float32 lengthY = static_cast<float32>(dims.getY()) / static_cast<float32>(numberOfPartitionsPerAxis.getY()) * spacing[1];
  float32 lengthZ = static_cast<float32>(dims.getZ()) / static_cast<float32>(numberOfPartitionsPerAxis.getZ()) * spacing[2];
  return Result<FloatVec3>{FloatVec3(lengthX, lengthY, lengthZ)};
}

Result<FloatVec3> GeometryUtilities::CalculatePartitionLengthsByPartitionCount(const RectGridGeom& geometry, const SizeVec3& numberOfPartitionsPerAxis)
{
  const Float32Array* xBoundsPtr = geometry.getXBounds();
  const Float32Array* yBoundsPtr = geometry.getYBounds();
  const Float32Array* zBoundsPtr = geometry.getZBounds();

  if(xBoundsPtr == nullptr)
  {
    return MakeErrorResult<FloatVec3>(-4000, "Unable to calculate partition lengths using the partition count - X Bounds array is not available.");
  }

  if(yBoundsPtr == nullptr)
  {
    return MakeErrorResult<FloatVec3>(-4001, "Unable to calculate partition lengths using the partition count - Y Bounds array is not available.");
  }

  if(zBoundsPtr == nullptr)
  {
    return MakeErrorResult<FloatVec3>(-4002, "Unable to calculate partition lengths using the partition count - Z Bounds array is not available.");
  }

  if(xBoundsPtr->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3>(-4003, "Unable to calculate partition lengths using the partition count - X Bounds array is empty.");
  }

  if(yBoundsPtr->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3>(-4004, "Unable to calculate partition lengths using the partition count - Y Bounds array is empty.");
  }

  if(zBoundsPtr->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3>(-4005, "Unable to calculate partition lengths using the partition count - Z Bounds array is empty.");
  }

  FloatVec3 lengthPerPartition = {0.0f, 0.0f, 0.0f};

  const AbstractDataStore<float32>& xStore = xBoundsPtr->getDataStoreRef();
  float32 maxX = xStore.getValue(xBoundsPtr->getNumberOfTuples() - 1);
  lengthPerPartition.setX((maxX - xStore.getValue(0)) / static_cast<float32>(numberOfPartitionsPerAxis.getX()));

  const AbstractDataStore<float32>& yStore = yBoundsPtr->getDataStoreRef();
  float32 maxY = yStore.getValue(yBoundsPtr->getNumberOfTuples() - 1);
  lengthPerPartition.setY((maxY - yStore.getValue(0)) / static_cast<float32>(numberOfPartitionsPerAxis.getY()));

  const AbstractDataStore<float32>& zStore = zBoundsPtr->getDataStoreRef();
  float32 maxZ = zStore.getValue(zBoundsPtr->getNumberOfTuples() - 1);
  lengthPerPartition.setZ((maxZ - zStore.getValue(0)) / static_cast<float32>(numberOfPartitionsPerAxis.getZ()));

  return Result<FloatVec3>{lengthPerPartition};
}

Result<FloatVec3> GeometryUtilities::CalculateNodeBasedPartitionSchemeOrigin(const INodeGeometry0D& geometry)
{
  const auto boundingBox = GetPaddedNodePartitionBounds(geometry);
  if(!boundingBox.isValid())
  {
    return {};
  }
  return Result<FloatVec3>{FloatVec3(boundingBox.getMinPoint())};
}

Result<FloatVec3> GeometryUtilities::CalculatePartitionLengthsOfBoundingBox(const BoundingBox3Df& boundingBox, const SizeVec3& numberOfPartitionsPerAxis)
{
  const auto& min = boundingBox.getMinPoint();
  const auto& max = boundingBox.getMaxPoint();
  // Calculate the length per partition for each dimension, and set it into the partitioning scheme image geometry
  float32 lengthX = ((max[0] - min[0]) / static_cast<float32>(numberOfPartitionsPerAxis.getX()));
  float32 lengthY = ((max[1] - min[1]) / static_cast<float32>(numberOfPartitionsPerAxis.getY()));
  float32 lengthZ = ((max[2] - min[2]) / static_cast<float32>(numberOfPartitionsPerAxis.getZ()));
  FloatVec3 lengthPerPartition = {lengthX, lengthY, lengthZ};
  return Result<FloatVec3>{lengthPerPartition};
}

/**
 * @brief The ComputeTriangleAreasImpl class implements a threaded algorithm that computes the area of each
 * triangle for a set of triangles
 */
class ComputeTriangleAreasImpl
{
public:
  ComputeTriangleAreasImpl(const TriangleGeom* triangleGeom, Float64AbstractDataStore& areas, const std::atomic_bool& shouldCancel)
  : m_TriangleGeom(triangleGeom)
  , m_Areas(areas)
  , m_ShouldCancel(shouldCancel)
  {
  }
  virtual ~ComputeTriangleAreasImpl() = default;

  void convert(size_t start, size_t end) const
  {
    for(size_t triangleIndex = start; triangleIndex < end; triangleIndex++)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      std::array<FloatVec3, 3> vertCoords;
      m_TriangleGeom->getFaceCoordinates(triangleIndex, vertCoords);
      m_Areas[triangleIndex] = 0.5F * (vertCoords[0] - vertCoords[1]).cross(vertCoords[0] - vertCoords[2]).magnitude();
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const TriangleGeom* m_TriangleGeom = nullptr;
  Float64AbstractDataStore& m_Areas;
  const std::atomic_bool& m_ShouldCancel;
};

Result<> GeometryUtilities::ComputeTriangleAreas(const nx::core::TriangleGeom* triangleGeom, Float64AbstractDataStore& faceAreas, const std::atomic_bool& shouldCancel)
{
  // Parallel algorithm to find duplicate nodes
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(triangleGeom->getNumberOfFaces()));
  dataAlg.execute(ComputeTriangleAreasImpl(triangleGeom, faceAreas, shouldCancel));

  return {};
}

/**
 * @brief The CalculateAreasImpl class implements a threaded algorithm that computes the normal of each
 * triangle for a set of triangles
 */
class CalculateNormalsImpl
{
public:
  CalculateNormalsImpl(const TriangleGeom* triangleGeom, Float64AbstractDataStore& normals, const std::atomic_bool& shouldCancel)
  : m_TriangleGeom(triangleGeom)
  , m_Normals(normals)
  , m_ShouldCancel(shouldCancel)
  {
  }
  virtual ~CalculateNormalsImpl() = default;

  void generate(size_t start, size_t end) const
  {
    for(size_t triangleIndex = start; triangleIndex < end; triangleIndex++)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      std::array<FloatVec3, 3> vertCoords;
      m_TriangleGeom->getFaceCoordinates(triangleIndex, vertCoords);

      auto normal = (vertCoords[1] - vertCoords[0]).cross(vertCoords[2] - vertCoords[0]);
      normal = normal / normal.magnitude();

      m_Normals[triangleIndex * 3] = static_cast<float64>(normal[0]);
      m_Normals[triangleIndex * 3 + 1] = static_cast<float64>(normal[1]);
      m_Normals[triangleIndex * 3 + 2] = static_cast<float64>(normal[2]);
    }
  }

  void operator()(const Range& range) const
  {
    generate(range.min(), range.max());
  }

private:
  const TriangleGeom* m_TriangleGeom = nullptr;
  Float64AbstractDataStore& m_Normals;
  const std::atomic_bool& m_ShouldCancel;
};

Result<> GeometryUtilities::ComputeTriangleNormals(const nx::core::TriangleGeom* triangleGeom, Float64AbstractDataStore& normals, const std::atomic_bool& shouldCancel)
{
  const bool usesOutOfCoreStore = AnyOutOfCore({triangleGeom->getFaces(), triangleGeom->getVertices()}) || normals.getStoreType() == IDataStore::StoreType::OutOfCore;
  const bool useOutOfCoreAlgorithm = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCoreAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  if(useOutOfCoreAlgorithm)
  {
    // Preserve this utility's float arithmetic, which differs from the Eigen double path.
    return MeshingUtilities::detail::CalculateNormalsInBlocks(triangleGeom->getFaces()->getDataStoreRef(), triangleGeom->getVertices()->getDataStoreRef(), normals, shouldCancel,
                                                              [](const float32* a, const float32* b, const float32* c) -> std::array<float64, 3> {
                                                                const std::array<FloatVec3, 3> vertCoords = {FloatVec3{a[0], a[1], a[2]}, FloatVec3{b[0], b[1], b[2]}, FloatVec3{c[0], c[1], c[2]}};
                                                                auto normal = (vertCoords[1] - vertCoords[0]).cross(vertCoords[2] - vertCoords[0]);
                                                                normal = normal / normal.magnitude();
                                                                return {static_cast<float64>(normal[0]), static_cast<float64>(normal[1]), static_cast<float64>(normal[2])};
                                                              });
  }

  // Parallel algorithm to find duplicate nodes
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(triangleGeom->getNumberOfFaces()));
  dataAlg.execute(CalculateNormalsImpl(triangleGeom, normals, shouldCancel));

  return {};
}

namespace slice_helper
{
// Define a small epsilon for floating-point comparisons
const float EPSILON = 1e-6f;
// Edge Structure: Pair of points representing a line segment
struct Edge
{
  FloatVec3 start;
  FloatVec3 end;
  bool valid;
  int32 region_id;
  uint8 positive_count = 0;
  uint8 negative_count = 0;
  uint8 zeroCount = 0;

  // Constructors
  Edge()
  : valid(false)
  , region_id(0)
  {
  }
  Edge(FloatVec3 start_, FloatVec3 end_)
  : start(std::move(start_))
  , end(std::move(end_))
  , valid(true)
  , region_id(0)
  {
  }

  bool operator==(const Edge& e) const
  {
    auto diffStart = e.start - start;
    auto diffEnd = e.end - end;

    return (diffStart[0] < EPSILON && diffStart[1] < EPSILON && diffStart[2] < EPSILON && diffEnd[0] < EPSILON && diffEnd[1] < EPSILON && diffEnd[2] < EPSILON);
  }

  bool operator<(const Edge& e) const
  {
    return !(e == *this);
  }
};

// Plane Structure: Defined by a normal vector and a distance from the origin (plane constant)
struct Plane
{
  FloatVec3 normal; // Should be normalized
  float d;          // Plane constant

  // Construct a plane from a normal and a point on the plane
  Plane(const FloatVec3& normal_, const FloatVec3& point)
  {
    normal = normal_;
    d = -normal.dot(point);
  }

  // Compute signed distance from a point to the plane
  float signedDistance(const FloatVec3& point) const
  {
    return normal.dot(point) + d;
  }
};

/**
 * @brief
 */
struct PointInfo
{
  float signed_distance;
  uint8 location;

  explicit PointInfo(float signedDistance)
  : signed_distance(signedDistance)
  , location(3) // Default the point is on the plane
  {
    if(signed_distance > EPSILON)
    {
      location = 1; // Above the plane
    }
    else if(signed_distance < -EPSILON)
    {
      location = 2; // Below the plane
    }
  }

  bool positive() const
  {
    return location == 1;
  }
  bool negative() const
  {
    return location == 2;
  }
  bool onPlane() const
  {
    return location == 3;
  }
  bool planeSplitsEdge(const PointInfo& pi) const
  {
    return (location == 1 && pi.location == 2) || (location == 2 && pi.location == 1);
  }
};

// ----------------------------------------------------------------------------
// Function to compute the intersection between a triangle and a plane
static Edge IntersectTriangleWithPlane(const FloatVec3& v0, const FloatVec3& v1, const FloatVec3& v2, const Plane& plane, float zPlane)
{
  PointInfo p0{plane.signedDistance(v0)};
  PointInfo p1{plane.signedDistance(v1)};
  PointInfo p2{plane.signedDistance(v2)};

  // Count the number of vertices on each side of the plane
  int positiveCount = static_cast<int>(p0.positive()) + static_cast<int>(p1.positive()) + static_cast<int>(p2.positive());
  int negativeCount = static_cast<int>(p0.negative()) + static_cast<int>(p1.negative()) + static_cast<int>(p2.negative());
  int zeroCount = static_cast<int>(p0.onPlane()) + static_cast<int>(p1.onPlane()) + static_cast<int>(p2.onPlane());

  // No intersection if all vertices are on one side of the plane
  // Handle case where the triangle lies entirely on the plane
  if(positiveCount == 3 || negativeCount == 3 || zeroCount == 3)
  {
    Edge e;
    e.positive_count = positiveCount;
    e.negative_count = negativeCount;
    e.zeroCount = zeroCount;
    return e; // Invalid edge because triangle is completely above or below the plane
  }

  // Edge to store the intersection line segment
  Edge intersectionEdge;
  intersectionEdge.positive_count = positiveCount;
  intersectionEdge.negative_count = negativeCount;
  intersectionEdge.zeroCount = zeroCount;

  // Interpolate from the lower-z endpoint so shared edges produce identical points.
  auto computeIntersection = [](const Edge& e, float d1, float d2, float planeZ) -> FloatVec3 {
    FloatVec3 s = e.start;
    FloatVec3 en = e.end;
    if(s[2] > en[2])
    {
      std::swap(s, en);
      std::swap(d1, d2);
    }
    const float t = d1 / (d1 - d2);
    FloatVec3 p = s + (en - s) * t;
    p[2] = planeZ;
    return p;
  };

  // Handle cases where only one intersection point is found (vertex lies on plane)
  // and the other two vertices are either both above or below the plane
  // Find the vertex that lies on the plane
  if(zeroCount == 1 && (positiveCount == 2 || negativeCount == 2))
  {
    Edge e;
    e.positive_count = positiveCount;
    e.negative_count = negativeCount;
    e.zeroCount = zeroCount;
    return e;
  }

  if(positiveCount == 1 && negativeCount == 1 && zeroCount == 1)
  {
    Edge e;
    if(p0.onPlane())
    {
      e.start = v0;
      e.end = computeIntersection({v1, v2}, p1.signed_distance, p2.signed_distance, zPlane);
    }
    else if(p1.onPlane())
    {
      e.start = v1;
      e.end = computeIntersection({v0, v2}, p0.signed_distance, p2.signed_distance, zPlane);
    }
    else if(p2.onPlane())
    {
      e.start = v2;
      e.end = computeIntersection({v0, v1}, p0.signed_distance, p1.signed_distance, zPlane);
    }
    e.start[2] = zPlane;
    e.positive_count = positiveCount;
    e.negative_count = negativeCount;
    e.zeroCount = zeroCount;
    e.valid = true;
    return e;
  }

  // Check edges for coincidence with plane
  if(p0.onPlane() && p1.onPlane() && zeroCount == 2)
  {
    Edge e{v0, v1};
    e.start[2] = zPlane;
    e.end[2] = zPlane;
    e.positive_count = positiveCount;
    e.negative_count = negativeCount;
    e.zeroCount = zeroCount;
    return e;
  }
  if(p1.onPlane() && p2.onPlane() && zeroCount == 2)
  {
    Edge e{v1, v2};
    e.start[2] = zPlane;
    e.end[2] = zPlane;
    e.positive_count = positiveCount;
    e.negative_count = negativeCount;
    e.zeroCount = zeroCount;
    return e;
  }
  if(p0.onPlane() && p2.onPlane() && zeroCount == 2)
  {
    Edge e{v0, v2};
    e.start[2] = zPlane;
    e.end[2] = zPlane;
    e.positive_count = positiveCount;
    e.negative_count = negativeCount;
    e.zeroCount = zeroCount;
    return e;
  }

  if(p0.planeSplitsEdge(p1) && p0.planeSplitsEdge(p2))
  {
    auto intersectionPoint0 = computeIntersection({v0, v1}, p0.signed_distance, p1.signed_distance, zPlane);
    auto intersectionPoint1 = computeIntersection({v0, v2}, p0.signed_distance, p2.signed_distance, zPlane);

    Edge e{intersectionPoint0, intersectionPoint1};
    e.positive_count = positiveCount;
    e.negative_count = negativeCount;
    e.zeroCount = zeroCount;
    return e;
  }

  if(p0.planeSplitsEdge(p1) && p1.planeSplitsEdge(p2))
  {
    auto intersectionPoint0 = computeIntersection({v0, v1}, p0.signed_distance, p1.signed_distance, zPlane);
    auto intersectionPoint1 = computeIntersection({v1, v2}, p1.signed_distance, p2.signed_distance, zPlane);
    Edge e{intersectionPoint0, intersectionPoint1};
    e.positive_count = positiveCount;
    e.negative_count = negativeCount;
    e.zeroCount = zeroCount;
    return e;
  }

  if(p1.planeSplitsEdge(p2) && p2.planeSplitsEdge(p0))
  {
    auto intersectionPoint0 = computeIntersection({v1, v2}, p1.signed_distance, p2.signed_distance, zPlane);
    auto intersectionPoint1 = computeIntersection({v2, v0}, p2.signed_distance, p0.signed_distance, zPlane);
    Edge e{intersectionPoint0, intersectionPoint1};
    e.positive_count = positiveCount;
    e.negative_count = negativeCount;
    e.zeroCount = zeroCount;
    return e;
  }

  // No valid intersection found
  Edge e;
  e.positive_count = positiveCount;
  e.negative_count = negativeCount;
  e.zeroCount = zeroCount;
  return e; // Invalid edge
}
} // namespace slice_helper

// ----------------------------------------------------------------------------
//
usize GeometryUtilities::determineBoundsAndNumSlices(float32& minDim, float32& maxDim, usize numTris, AbstractDataStore<INodeGeometry2D::SharedFaceList::value_type>& tris,
                                                     AbstractDataStore<INodeGeometry0D::SharedVertexList::value_type>& triVerts, uint64 sliceRange, float32 zStart, float32 zEnd,
                                                     float32 sliceResolution)
{
  for(usize i = 0; i < numTris; i++)
  {
    for(usize j = 0; j < 3; j++)
    {
      const usize vert = tris[3 * i + j];
      minDim = std::min<nx::core::types::float32>(minDim, triVerts[3 * vert + 2]);
      maxDim = std::max<nx::core::types::float32>(maxDim, triVerts[3 * vert + 2]);
    }
  }

  // adjust sectioning range if user selected a specific range - check that user range is within actual range
  if(sliceRange == k_UserDefinedRange)
  {
    minDim = zStart;
    maxDim = zEnd;
  }
  // Empty geometry or invalid bounds cannot produce a valid slice count.
  if(numTris == 0 || !(maxDim >= minDim))
  {
    return 0;
  }

  const float64 span = static_cast<float64>(maxDim) - static_cast<float64>(minDim);
  const float64 ratio = span / static_cast<float64>(sliceResolution);
  // Scale the tolerance to include the end plane for whole multiples of the spacing.
  const float64 tolerance = std::max(1.0E-5, ratio * 1.0E-6);
  return static_cast<usize>(std::floor(ratio + tolerance)) + 1;
}

// ----------------------------------------------------------------------------
//
using TriStore = AbstractDataStore<INodeGeometry2D::SharedFaceList::value_type>;
using VertsStore = AbstractDataStore<INodeGeometry0D::SharedVertexList::value_type>;

inline static std::array<nx::core::FloatVec3, 3> GetFaceCoordinates(usize triangleId, VertsStore& verts, TriStore& triangleList)
{
  usize v0Idx = triangleList[triangleId * 3];
  usize v1Idx = triangleList[triangleId * 3 + 1];
  usize v2Idx = triangleList[triangleId * 3 + 2];
  return {FloatVec3{verts[v0Idx * 3], verts[v0Idx * 3 + 1], verts[v0Idx * 3 + 2]}, FloatVec3{verts[v1Idx * 3], verts[v1Idx * 3 + 1], verts[v1Idx * 3 + 2]},
          FloatVec3{verts[v2Idx * 3], verts[v2Idx * 3 + 1], verts[v2Idx * 3 + 2]}};
}

// ----------------------------------------------------------------------------
//
GeometryUtilities::SliceTriangleReturnType GeometryUtilities::SliceTriangleGeometry(nx::core::TriangleGeom& triangle, const std::atomic_bool& shouldCancel, uint64 sliceRange, float32 zStart,
                                                                                    float32 zEnd, float32 sliceSpacing, AbstractDataStore<int32>* triRegionIdPtr)
{
  if(!(sliceSpacing > 0.0f))
  {
    return {};
  }

  // Get the Abstract Data Store Classes
  TriStore& triEdgeStore = triangle.getFaces()->getDataStoreRef();
  VertsStore& triVertStore = triangle.getVertices()->getDataStoreRef();
  usize numTris = triangle.getNumberOfFaces();

  // determine bounds and number of slices needed for CAD geometry
  float32 d = 0;
  float32 minZValue = std::numeric_limits<float32>::max();
  float32 maxZValue = -minZValue;
  usize numberOfSlices = determineBoundsAndNumSlices(minZValue, maxZValue, numTris, triEdgeStore, triVertStore, sliceRange, zStart, zEnd, sliceSpacing);

  std::vector<float32> slicedVerts;
  std::vector<int32> sliceIds;
  std::vector<int32> regionIds;

  int32 sliceIndex = -1;
  // Loop over each slice plane
  for(usize sliceIdx = 0; sliceIdx < numberOfSlices; sliceIdx++)
  {
    if(shouldCancel)
    {
      break;
    }
    const auto zValue = static_cast<float32>(std::min(static_cast<float64>(minZValue) + static_cast<float64>(sliceIdx) * static_cast<float64>(sliceSpacing), static_cast<float64>(maxZValue)));
    sliceIndex = static_cast<int32>(sliceIdx);
    d = zValue;

    // Define a plane with a normal vector and a point on the plane
    FloatVec3 planeNormal(0.0f, 0.0f, 1.0f); // Plane normal pointing along +Z
    FloatVec3 pointOnPlane(0.0f, 0.0f, d);   // Plane passes through current Z Plane

    // Create the plane
    slice_helper::Plane plane(planeNormal, pointOnPlane);

    // Loop over each Triangle and get edges/vertices of any intersection
    for(usize triIdx = 0; triIdx < numTris; triIdx++)
    {
      int32 regionId = 0;
      // get regionId of this triangle (if they are available)
      if(nullptr != triRegionIdPtr)
      {
        regionId = triRegionIdPtr->operator[](triIdx);
      }
      std::array<nx::core::FloatVec3, 3> faceVertices = GetFaceCoordinates(triIdx, triVertStore, triEdgeStore);

      // Compute the intersection
      slice_helper::Edge intersectionEdge = IntersectTriangleWithPlane(faceVertices[0], faceVertices[1], faceVertices[2], plane, zValue);
      if(intersectionEdge.valid)
      {
        slicedVerts.push_back(intersectionEdge.start[0]);
        slicedVerts.push_back(intersectionEdge.start[1]);
        slicedVerts.push_back(intersectionEdge.start[2]);

        slicedVerts.push_back(intersectionEdge.end[0]);
        slicedVerts.push_back(intersectionEdge.end[1]);
        slicedVerts.push_back(intersectionEdge.end[2]);

        sliceIds.push_back(sliceIndex);
        if(nullptr != triRegionIdPtr)
        {
          regionIds.push_back(regionId);
        }
      }
    } // END TRIANGLE LOOP
  } // END SLICE LOOP

  return {.SliceVerts = std::move(slicedVerts), .SliceIds = std::move(sliceIds), .RegionIds = std::move(regionIds), .NumberOfSlices = numberOfSlices};
}

// -----------------------------------------------------------------------------
Result<> GeometryUtilities::ValidateFiniteVector(nonstd::span<const float32> values, std::string_view parameterName, int32 errorCode)
{
  for(usize axis = 0; axis < values.size(); ++axis)
  {
    if(!std::isfinite(values[axis]))
    {
      return MakeErrorResult(errorCode,
                             fmt::format("Parameter '{}' {} value {} must be finite.", parameterName, axis < 3 ? std::string(1, "XYZ"[axis]) : fmt::format("component {}", axis), values[axis]));
    }
  }
  return {};
}

// -----------------------------------------------------------------------------
Result<> GeometryUtilities::ValidatePositiveFiniteVector(nonstd::span<const float32> values, std::string_view parameterName, int32 errorCode)
{
  for(usize axis = 0; axis < values.size(); ++axis)
  {
    if(!std::isfinite(values[axis]) || !(values[axis] > 0))
    {
      return MakeErrorResult(errorCode, fmt::format("Parameter '{}' {} value {} must be finite and greater than 0.", parameterName,
                                                    axis < 3 ? std::string(1, "XYZ"[axis]) : fmt::format("component {}", axis), values[axis]));
    }
  }
  return {};
}

// -----------------------------------------------------------------------------
Result<SizeVec3> GeometryUtilities::ComputePaddedDimensions(const SizeVec3& dims, const std::array<std::array<int32, 2>, 3>& padding, const std::array<bool, 3>& active)
{
  SizeVec3 output = dims;
  constexpr usize k_Max = std::numeric_limits<usize>::max();
  for(usize axis = 0; axis < 3; ++axis)
  {
    if(!active[axis])
    {
      continue;
    }
    for(usize side = 0; side < 2; ++side)
    {
      const usize amount = static_cast<usize>(padding[axis][side]);
      if(amount > k_Max - output[axis])
      {
        return MakeErrorResult<SizeVec3>(k_PaddedExtentOverflowError, fmt::format("Parameter '{} Min & Max Padding' {} value {} with dimension {} produces an extent above {}.", "XYZ"[axis],
                                                                                  side == 0 ? "Min" : "Max", amount, output[axis], k_Max));
      }
      output[axis] += amount;
    }
  }
  if(output[0] == 0 || output[1] == 0 || output[2] == 0)
  {
    return {output};
  }
  usize product = 1;
  for(usize axis = 0; axis < 3; ++axis)
  {
    if(output[axis] > k_Max / product)
    {
      return MakeErrorResult<SizeVec3>(k_PaddedTupleOverflowError,
                                       fmt::format("Parameter '{} Min & Max Padding' values [{}, {}] with dimension {} produce padded {} extent {} and a tuple product above {}.", "XYZ"[axis],
                                                   padding[axis][0], padding[axis][1], dims[axis], "XYZ"[axis], output[axis], k_Max));
    }
    product *= output[axis];
  }
  return {output};
}
