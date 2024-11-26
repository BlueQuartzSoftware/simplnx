#include "GeometryUtilities.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"

using namespace nx::core;

namespace
{
constexpr float32 k_PartitionEdgePadding = 0.000001;
const Point3Df k_Padding(k_PartitionEdgePadding, k_PartitionEdgePadding, k_PartitionEdgePadding);
} // namespace

GeometryUtilities::FindUniqueIdsImpl::FindUniqueIdsImpl(VertexStore& vertexStore, const std::vector<std::vector<size_t>>& nodesInBin, nx::core::Int64DataStore& uniqueIds)
: m_VertexStore(vertexStore)
, m_NodesInBin(nodesInBin)
, m_UniqueIds(uniqueIds)
{
}

// -----------------------------------------------------------------------------
void GeometryUtilities::FindUniqueIdsImpl::convert(size_t start, size_t end) const
{
  int64* uniqueIdsPtr = m_UniqueIds.data();
  for(size_t i = start; i < end; i++)
  {
    for(size_t j = 0; j < m_NodesInBin[i].size(); j++)
    {
      size_t node1 = m_NodesInBin[i][j];
      if(uniqueIdsPtr[node1] == static_cast<int64_t>(node1))
      {
        for(size_t k = j + 1; k < m_NodesInBin[i].size(); k++)
        {
          size_t node2 = m_NodesInBin[i][k];
          if(m_VertexStore[node1 * 3] == m_VertexStore[node2 * 3] && m_VertexStore[node1 * 3 + 1] == m_VertexStore[node2 * 3 + 1] && m_VertexStore[node1 * 3 + 2] == m_VertexStore[node2 * 3 + 2])
          {
            uniqueIdsPtr[node2] = node1;
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

Result<FloatVec3> GeometryUtilities::CalculatePartitionLengthsByPartitionCount(const INodeGeometry0D& geometry, const SizeVec3& numberOfPartitionsPerAxis)
{
  BoundingBox3Df boundingBox = geometry.getBoundingBox();
  if(!boundingBox.isValid())
  {
    return {};
  }
  return GeometryUtilities::CalculatePartitionLengthsOfBoundingBox({(boundingBox.getMinPoint() - k_Padding), (boundingBox.getMaxPoint() + k_Padding)}, numberOfPartitionsPerAxis);
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
  const Float32Array* xBounds = geometry.getXBounds();
  const Float32Array* yBounds = geometry.getYBounds();
  const Float32Array* zBounds = geometry.getZBounds();

  if(xBounds == nullptr)
  {
    return MakeErrorResult<FloatVec3>(-4000, "Unable to calculate partition lengths using the partition count - X Bounds array is not available.");
  }

  if(yBounds == nullptr)
  {
    return MakeErrorResult<FloatVec3>(-4001, "Unable to calculate partition lengths using the partition count - Y Bounds array is not available.");
  }

  if(zBounds == nullptr)
  {
    return MakeErrorResult<FloatVec3>(-4002, "Unable to calculate partition lengths using the partition count - Z Bounds array is not available.");
  }

  if(xBounds->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3>(-4003, "Unable to calculate partition lengths using the partition count - X Bounds array is empty.");
  }

  if(yBounds->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3>(-4004, "Unable to calculate partition lengths using the partition count - Y Bounds array is empty.");
  }

  if(zBounds->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3>(-4005, "Unable to calculate partition lengths using the partition count - Z Bounds array is empty.");
  }

  FloatVec3 lengthPerPartition = {0.0f, 0.0f, 0.0f};

  const AbstractDataStore<float32>& xStore = xBounds->getDataStoreRef();
  float32 maxX = xStore.getValue(xBounds->getNumberOfTuples() - 1);
  lengthPerPartition.setX(maxX / static_cast<float32>(numberOfPartitionsPerAxis.getX()));

  const AbstractDataStore<float32>& yStore = yBounds->getDataStoreRef();
  float32 maxY = yStore.getValue(yBounds->getNumberOfTuples() - 1);
  lengthPerPartition.setY(maxY / static_cast<float32>(numberOfPartitionsPerAxis.getY()));

  const AbstractDataStore<float32>& zStore = yBounds->getDataStoreRef();
  float32 maxZ = zStore.getValue(zBounds->getNumberOfTuples() - 1);
  lengthPerPartition.setZ(maxZ / static_cast<float32>(numberOfPartitionsPerAxis.getZ()));

  return Result<FloatVec3>{lengthPerPartition};
}

Result<FloatVec3> GeometryUtilities::CalculateNodeBasedPartitionSchemeOrigin(const INodeGeometry0D& geometry)
{
  BoundingBox3Df boundingBox = geometry.getBoundingBox();
  if(!boundingBox.isValid())
  {
    return {};
  }
  return Result<FloatVec3>{FloatVec3(boundingBox.getMinPoint() - k_Padding)};
}

Result<FloatVec3> GeometryUtilities::CalculatePartitionLengthsOfBoundingBox(const BoundingBox3Df& boundingBox, const SizeVec3& numberOfPartitionsPerAxis)
{
  auto min = boundingBox.getMinPoint();
  auto max = boundingBox.getMaxPoint();
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
    std::array<float, 3> cross = {0.0f, 0.0f, 0.0f};
    for(size_t triangleIndex = start; triangleIndex < end; triangleIndex++)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      std::array<Point3Df, 3> vertCoords;
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
      std::array<Point3Df, 3> vertCoords;
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
  Point3Df start;
  Point3Df end;
  bool valid;
  int32 regionId;

  // Constructors
  Edge()
  : valid(false)
  , regionId(0)
  {
  }
  Edge(const Point3Df& start_, const Point3Df& end_)
  : start(start_)
  , end(end_)
  , valid(true)
  , regionId(0)
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
  Point3Df normal; // Should be normalized
  float d;         // Plane constant

  // Construct a plane from a normal and a point on the plane
  Plane(const Point3Df& normal_, const Point3Df& point)
  {
    normal = normal_;
    d = -normal.dot(point);
  }

  // Compute signed distance from a point to the plane
  float signedDistance(const Point3Df& point) const
  {
    return normal.dot(point) + d;
  }
};

struct PointInfo
{

  float SignedDistance;
  uint8 location;

  PointInfo(float signedDistance)
  : SignedDistance(signedDistance)
  , location(3) // Default the point is on the plane
  {
    if(SignedDistance > EPSILON)
    {
      location = 1; // Above the plane
    }
    else if(SignedDistance < EPSILON)
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
Edge IntersectTriangleWithPlane(const Point3Df& v0, const Point3Df& v1, const Point3Df& v2, const Plane& plane)
{
  PointInfo p0 = {plane.signedDistance(v0)};
  PointInfo p1 = {plane.signedDistance(v1)};
  PointInfo p2 = {plane.signedDistance(v2)};

  // Count the number of vertices on each side of the plane
  int positiveCount = p0.positive() + p1.positive() + p2.positive();
  int negativeCount = p0.negative() + p1.negative() + p2.negative();
  int zeroCount = p0.onPlane() + p1.onPlane() + p2.onPlane();

  // No intersection if all vertices are on one side of the plane
  if(positiveCount == 3 || negativeCount == 3)
  {
    return {}; // Invalid edge because triangle is completely above or below the plane
  }

  // Edge to store the intersection line segment
  Edge intersectionEdge;
  // int intersectionCount = 0;

  // Helper lambda to compute intersection point
  auto computeIntersection = [](const Edge& e, float _dist1, float _dist2) -> Point3Df {
    float t = _dist1 / (_dist1 - _dist2);   // Compute interpolation parameter
    return e.start + (e.end - e.start) * t; // Return the interpolated point
  };

  // Handle cases where only one intersection point is found (vertex lies on plane)
  // Find the vertex that lies on the plane

  if(p0.onPlane() && zeroCount == 1)
  {
    return {v0, v0};
  }
  else if(p1.onPlane() && zeroCount == 1)
  {
    return {v1, v1};
  }
  else if(p2.onPlane() && zeroCount == 1)
  {
    return {v2, v2};
  }

  // Check edges for coincidence with plane
  if(p0.onPlane() && p1.onPlane())
  {
    return {v0, v1};
  }
  if(p1.onPlane() && p2.onPlane())
  {
    return {v1, v2};
  }
  if(p0.onPlane() && p2.onPlane())
  {
    return {v0, v2};
  }

  // Handle case where the triangle lies entirely on the plane
  if(zeroCount == 3)
  {
    // Return any edge of the triangle
    return {v0, v1};
  }

  if(p0.planeSplitsEdge(p1) && p0.planeSplitsEdge(p2))
  {
    auto intersectionPoint0 = computeIntersection({v0, v1}, p0.SignedDistance, p1.SignedDistance);
    auto intersectionPoint1 = computeIntersection({v0, v2}, p0.SignedDistance, p2.SignedDistance);
    return {intersectionPoint0, intersectionPoint1};
  }

  if(p0.planeSplitsEdge(p1) && p1.planeSplitsEdge(p2))
  {
    auto intersectionPoint0 = computeIntersection({v0, v1}, p0.SignedDistance, p1.SignedDistance);
    auto intersectionPoint1 = computeIntersection({v1, v2}, p1.SignedDistance, p2.SignedDistance);
    return {intersectionPoint0, intersectionPoint1};
  }

  if(p1.planeSplitsEdge(p2) && p2.planeSplitsEdge(p0))
  {
    auto intersectionPoint0 = computeIntersection({v1, v2}, p1.SignedDistance, p2.SignedDistance);
    auto intersectionPoint1 = computeIntersection({v2, v0}, p2.SignedDistance, p0.SignedDistance);
    return {intersectionPoint0, intersectionPoint1};
  }

  // No valid intersection found
  return {}; // Invalid edge
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
      if(minDim > triVerts[3 * vert + 2])
      {
        minDim = triVerts[3 * vert + 2];
      }
      if(maxDim < triVerts[3 * vert + 2])
      {
        maxDim = triVerts[3 * vert + 2];
      }
    }
  }

  // adjust sectioning range if user selected a specific range - check that user range is within actual range
  if(sliceRange == 1)
  {
    // if(m_InputValues->Zstart > minDim)
    {
      minDim = zStart;
    }
    // if(m_InputValues->Zend < maxDim)
    {
      maxDim = zEnd;
    }
  }
  // TODO: Is this still correct? Why have the subtraction at all?
  const auto numberOfSlices = static_cast<usize>((maxDim - minDim) / sliceResolution) + 1;
  // const auto numberOfSlices = static_cast<usize>((maxDim - minDim) / m_InputValues->SliceResolution) + 1;
  return numberOfSlices;
}
#if 0
GeometryUtilities::SliceTriangleReturnType GeometryUtilities::SliceTriangleGeometry2(nx::core::TriangleGeom& triangle, const std::atomic_bool& shouldCancel, uint64 sliceRange, float32 zStart,
                                                                                     float32 zEnd, float32 sliceResolution, AbstractDataStore<int32>* triRegionIdPtr)
{
  using TriStore = AbstractDataStore<INodeGeometry2D::SharedFaceList::value_type>;
  using VertsStore = AbstractDataStore<INodeGeometry0D::SharedVertexList::value_type>;

  // geometry will be rotated so that the sectioning direction is always 001 before rotating back
  std::array<float32, 3> n = {0.0f, 0.0f, 1.0f};

  TriStore& tris = triangle.getFaces()->getDataStoreRef();
  VertsStore& triVerts = triangle.getVertices()->getDataStoreRef();
  usize numTris = triangle.getNumberOfFaces();
  usize numTriVerts = triangle.getNumberOfVertices();

  // determine bounds and number of slices needed for CAD geometry
  nx::core::Point3Df q = {0.0f, 0.0f, 0.0f};
  nx::core::Point3Df r = {0.0f, 0.0f, 0.0f};
  nx::core::Point3Df p = {0.0f, 0.0f, 0.0f};
  nx::core::Point3Df corner = {0.0f, 0.0f, 0.0f};
  float32 d = 0;
  float32 minZValue = std::numeric_limits<float32>::max();
  float32 maxZValue = -minZValue;
  usize numberOfSlices = determineBoundsAndNumSlices(minZValue, maxZValue, numTris, tris, triVerts, sliceRange, zStart, zEnd, sliceResolution);
  const auto minSlice = static_cast<int64>(minZValue / sliceResolution);
  const auto maxSlice = static_cast<int64>(maxZValue / sliceResolution);

  std::vector<float32> slicedVerts;
  std::vector<int32> sliceIds;
  std::vector<int32> regionIds;

  int32 edgeCounter = 0;
  for(usize triIdx = 0; triIdx < numTris; triIdx++)
  {
    int32 regionId = 0;
    // get regionId of this triangle (if they are available)
    if(nullptr != triRegionIdPtr)
    {
      regionId = triRegionIdPtr->operator[](triIdx);
    }

    // determine which slices would hit the triangle

    // 1: Find the min and max z vertex value
    float32 zValue0 = triVerts[3 * tris[3 * triIdx + 0] + 2];
    float32 zValue1 = triVerts[3 * tris[3 * triIdx + 1] + 2];
    float32 zValue2 = triVerts[3 * tris[3 * triIdx + 2] + 2];
    float minCurrentTriVertexZValue = std::min(std::min(zValue0, zValue1), zValue2);
    float maxCurrentTriVertexZValue = std::max(std::max(zValue0, zValue1), zValue2);

    // 2: Check if this triangle would hit _any_ of the slices
    if(minCurrentTriVertexZValue > maxZValue || maxCurrentTriVertexZValue < minZValue)
    {
      continue;
    }

    // 3: Clamp the current triangle's min and max z vertex value to be within the global z min and max
    if(minCurrentTriVertexZValue < minZValue)
    {
      minCurrentTriVertexZValue = minZValue;
    }
    if(maxCurrentTriVertexZValue > maxZValue)
    {
      maxCurrentTriVertexZValue = maxZValue;
    }

    // 4: Compute the first and last slice planes and clamp to the min and max slice
    auto firstSlice = static_cast<int64>(minCurrentTriVertexZValue / sliceResolution);
    auto lastSlice = static_cast<int64>(maxCurrentTriVertexZValue / sliceResolution);
    if(firstSlice < minSlice)
    {
      firstSlice = minSlice;
    }
    if(lastSlice > maxSlice)
    {
      lastSlice = maxSlice;
    }

    std::array<Point3Df, 3> vertCoords;
    triangle.getFaceCoordinates(triIdx, vertCoords);
    auto normal = (vertCoords[1] - vertCoords[0]).cross(vertCoords[2] - vertCoords[0]);

    for(int64 j = firstSlice; j <= lastSlice; j++)
    {
      d = (sliceResolution * static_cast<float32>(j));

      std::array<nx::core::Point3Df, 3> faceVertices;
      triangle.getFaceCoordinates(triIdx, faceVertices);
      // Define a plane with a normal vector and a point on the plane
      Point3Df planeNormal(0.0f, 0.0f, 1.0f); // Plane normal pointing along +Z
      Point3Df pointOnPlane(0.0f, 0.0f, d);   // Plane passes through current Z Plane

      // Create the plane
      slice_helper::Plane plane(planeNormal, pointOnPlane);
      // Compute the intersection
      slice_helper::Edge intersectionEdge = IntersectTriangleWithPlane(faceVertices[0], faceVertices[1], faceVertices[2], plane);
      if(intersectionEdge.valid)
      {
        slicedVerts.push_back(intersectionEdge.start[0]);
        slicedVerts.push_back(intersectionEdge.start[1]);
        slicedVerts.push_back(intersectionEdge.start[2]);

        slicedVerts.push_back(intersectionEdge.end[0]);
        slicedVerts.push_back(intersectionEdge.end[1]);
        slicedVerts.push_back(intersectionEdge.end[2]);

        sliceIds.push_back(j - firstSlice);
        if(nullptr != triRegionIdPtr)
        {
          regionIds.push_back(regionId);
        }
        edgeCounter++;
      }
    }
  }

  return {std::move(slicedVerts), std::move(sliceIds), std::move(regionIds), numberOfSlices};
}
#endif

// ----------------------------------------------------------------------------
//
using TriStore = AbstractDataStore<INodeGeometry2D::SharedFaceList::value_type>;
using VertsStore = AbstractDataStore<INodeGeometry0D::SharedVertexList::value_type>;

inline std::array<nx::core::Point3Df, 3> GetFaceCoordinates(usize triangleId, VertsStore& verts, TriStore& triangleList)
{
  usize v0Idx = triangleList[triangleId * 3];
  usize v1Idx = triangleList[triangleId * 3 + 1];
  usize v2Idx = triangleList[triangleId * 3 + 2];
  return {Point3Df{verts[v0Idx * 3], verts[v0Idx * 3 + 1], verts[v0Idx * 3 + 2]}, Point3Df{verts[v1Idx * 3], verts[v1Idx * 3 + 1], verts[v1Idx * 3 + 2]},
          Point3Df{verts[v2Idx * 3], verts[v2Idx * 3 + 1], verts[v2Idx * 3 + 2]}};
}

// ----------------------------------------------------------------------------
//
GeometryUtilities::SliceTriangleReturnType GeometryUtilities::SliceTriangleGeometry(nx::core::TriangleGeom& triangle, const std::atomic_bool& shouldCancel, uint64 sliceRange, float32 zStart,
                                                                                    float32 zEnd, float32 sliceResolution, AbstractDataStore<int32>* triRegionIdPtr)
{
  // Get the Abstract Data Store Classes
  TriStore& triEdgeStore = triangle.getFaces()->getDataStoreRef();
  VertsStore& triVertStore = triangle.getVertices()->getDataStoreRef();
  usize numTris = triangle.getNumberOfFaces();

  // determine bounds and number of slices needed for CAD geometry
  float32 d = 0;
  float32 minZValue = std::numeric_limits<float32>::max();
  float32 maxZValue = -minZValue;
  usize numberOfSlices = determineBoundsAndNumSlices(minZValue, maxZValue, numTris, triEdgeStore, triVertStore, sliceRange, zStart, zEnd, sliceResolution);

  std::vector<float32> slicedVerts;
  std::vector<int32> sliceIds;
  std::vector<int32> regionIds;

  int32 edgeCounter = 0;
  int32 sliceIndex = 0;
  // Loop over each slice plane
  for(float zValue = zStart; zValue <= zEnd; zValue = zValue + sliceResolution)
  {
    d = zValue;

    // Define a plane with a normal vector and a point on the plane
    Point3Df planeNormal(0.0f, 0.0f, 1.0f); // Plane normal pointing along +Z
    Point3Df pointOnPlane(0.0f, 0.0f, d);   // Plane passes through current Z Plane

    // Create the plane
    slice_helper::Plane plane(planeNormal, pointOnPlane);
    int32 localEdgeCount = 0;
    std::set<slice_helper::Edge> uniqueEdges;
    // Loop over each Triangle and get edges/vertices of any intersection
    for(usize triIdx = 0; triIdx < numTris; triIdx++)
    {
      int32 regionId = 0;
      // get regionId of this triangle (if they are available)
      if(nullptr != triRegionIdPtr)
      {
        regionId = triRegionIdPtr->operator[](triIdx);
      }

      //       std::array<nx::core::Point3Df, 3> faceVertices;
      //       triangle.getFaceCoordinates(triIdx, faceVertices);

      std::array<nx::core::Point3Df, 3> faceVertices = GetFaceCoordinates(triIdx, triVertStore, triEdgeStore);

      // Compute the intersection
      slice_helper::Edge intersectionEdge = IntersectTriangleWithPlane(faceVertices[0], faceVertices[1], faceVertices[2], plane);

      if(intersectionEdge.valid)
      {
        uniqueEdges.insert(intersectionEdge);
      }

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
        edgeCounter++;
        localEdgeCount++;
      }
    }
    sliceIndex++;
  }

  return {std::move(slicedVerts), std::move(sliceIds), std::move(regionIds), numberOfSlices};
}
