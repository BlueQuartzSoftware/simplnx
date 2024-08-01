#include "GeometryMath.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

#include <stdexcept>

using namespace nx::core;

float32 nx::core::GeometryMath::AngleBetweenVectors(const nx::core::ZXZEuler& a, const nx::core::ZXZEuler& b)
{
  float norm1 = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
  float norm2 = std::sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
  float cosAng = (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / (norm1 * norm2);
  if(cosAng < -1.0f)
  {
    cosAng = -1.0f;
  }
  else if(cosAng > 1.0)
  {
    cosAng = 1.0;
  }
  return std::acos(cosAng);
}

BoundingBox3Df nx::core::GeometryMath::FindBoundingBoxOfVertices(INodeGeometry0D& geom)
{
  FloatVec3 ll = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
  FloatVec3 ur = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};

  const IGeometry::SharedVertexList& vertexList = geom.getVerticesRef();
  if(vertexList.getDataType() != DataType::float32)
  {
    return {ll, ur}; // will be invalid
  }

  auto& vertexListStore = vertexList.getIDataStoreRefAs<const DataStore<float32>>();

  for(size_t tuple = 0; tuple < vertexListStore.getNumberOfTuples(); tuple++)
  {
    float x = vertexListStore.getComponentValue(tuple, 0);
    ll[0] = (x < ll[0]) ? x : ll[0];
    ur[0] = (x > ur[0]) ? x : ur[0];

    float y = vertexListStore.getComponentValue(tuple, 1);
    ll[1] = (y < ll[1]) ? y : ll[1];
    ur[1] = (y > ur[1]) ? y : ur[1];

    float z = vertexListStore.getComponentValue(tuple, 2);
    ll[2] = (z < ll[2]) ? z : ll[2];
    ur[2] = (z > ur[2]) ? z : ur[2];
  }

  return {ll, ur}; // should be valid
}

BoundingBox3Df nx::core::GeometryMath::FindBoundingBoxOfFace(const detail::GeometryStoreCache& cache, const nx::core::TriangleGeom& triangleGeom, int32 faceId, std::vector<usize>& verts)
{
  // Unavoidable branch to verify integrity of subsequent function
  // specifically because this an exposed function, however, the
  // CPU should quickly be able to predict once it has run a few
  // iterations of the function making it relatively lightweight
  if(verts.size() != cache.NumVertsPerFace)
  {
    verts = std::vector<usize>(cache.NumVertsPerFace);
  }
  std::array<Point3Df, 3> points = GeometryMath::detail::GetFaceCoordinates<float32>(cache, faceId, verts);

  Point3Df ll = points[0];
  Point3Df ur = points[0];

  // Avoid Branch Prediction misses by exclusively doing multiplication operations
  // C++ (§4.5/4):
  // An r-value of type bool can be converted to an r-value of type int,
  // with false becoming zero and true becoming one.
  //
  // The standard guarantees the same for float
  bool swap = points[1][0] < ll[0];
  ll[0] = points[1][0] * static_cast<float32>(swap) + ll[0] * static_cast<float32>(!swap);

  swap = points[1][1] < ll[1];
  ll[1] = points[1][1] * static_cast<float32>(swap) + ll[1] * static_cast<float32>(!swap);

  swap = points[1][2] < ll[2];
  ll[2] = points[1][2] * static_cast<float32>(swap) + ll[2] * static_cast<float32>(!swap);

  swap = points[2][0] < ll[0];
  ll[0] = points[2][0] * static_cast<float32>(swap) + ll[0] * static_cast<float32>(!swap);

  swap = points[2][1] < ll[1];
  ll[1] = points[2][1] * static_cast<float32>(swap) + ll[1] * static_cast<float32>(!swap);

  swap = points[2][2] < ll[2];
  ll[2] = points[2][2] * static_cast<float32>(swap) + ll[2] * static_cast<float32>(!swap);

  swap = points[1][0] > ur[0];
  ur[0] = points[1][0] * static_cast<float32>(swap) + ur[0] * static_cast<float32>(!swap);

  swap = points[1][1] > ur[1];
  ur[1] = points[1][1] * static_cast<float32>(swap) + ur[1] * static_cast<float32>(!swap);

  swap = points[1][2] > ur[2];
  ur[2] = points[1][2] * static_cast<float32>(swap) + ur[2] * static_cast<float32>(!swap);

  swap = points[2][0] > ur[0];
  ur[0] = points[2][0] * static_cast<float32>(swap) + ur[0] * static_cast<float32>(!swap);

  swap = points[2][1] > ur[1];
  ur[1] = points[2][1] * static_cast<float32>(swap) + ur[1] * static_cast<float32>(!swap);

  swap = points[2][2] > ur[2];
  ur[2] = points[2][2] * static_cast<float32>(swap) + ur[2] * static_cast<float32>(!swap);

  return {ll, ur};
}

BoundingBox3Df nx::core::GeometryMath::FindBoundingBoxOfFaces(const nx::core::TriangleGeom& triangleGeom, const std::vector<int32>& faceIds)
{
  Point3Df ll(0, 0, 0);
  Point3Df ur(0, 0, 0);

  if(faceIds.empty())
  {
    return {ll, ur};
  }

  detail::GeometryStoreCache cache(triangleGeom.getVertices()->getDataStoreRef(), triangleGeom.getFaces()->getDataStoreRef(), triangleGeom.getNumberOfVerticesPerFace());

  // initialize temp storage 'verts' vector to avoid expensive
  // calls during tight loops below
  std::vector<usize> verts(cache.NumVertsPerFace);
  for(const auto& id : faceIds)
  {
    auto bounds = FindBoundingBoxOfFace(cache, triangleGeom, id, verts);
    Point3Df min = bounds.getMinPoint();
    Point3Df max = bounds.getMaxPoint();

    // Avoid Branch Prediction misses by exclusively doing multiplication operations
    // C++ (§4.5/4):
    // An r-value of type bool can be converted to an r-value of type int,
    // with false becoming zero and true becoming one.
    //
    // The standard guarantees the same for float
    bool swap = min[0] < ll[0];
    ll[0] = min[0] * static_cast<float32>(swap) + ll[0] * static_cast<float32>(!swap);

    swap = min[1] < ll[1];
    ll[1] = min[1] * static_cast<float32>(swap) + ll[1] * static_cast<float32>(!swap);

    swap = min[2] < ll[2];
    ll[2] = min[2] * static_cast<float32>(swap) + ll[2] * static_cast<float32>(!swap);

    swap = max[0] > ur[0];
    ur[0] = max[0] * static_cast<float32>(swap) + ur[0] * static_cast<float32>(!swap);

    swap = max[1] > ur[1];
    ur[1] = max[1] * static_cast<float32>(swap) + ur[1] * static_cast<float32>(!swap);

    swap = max[2] > ur[2];
    ur[2] = max[2] * static_cast<float32>(swap) + ur[2] * static_cast<float32>(!swap);
  }

  return {ll, ur};
}

BoundingBox3Df nx::core::GeometryMath::FindBoundingBoxOfRotatedFace(TriangleGeom& faces, int32 faceId, float32 g[3][3])
{
  throw std::runtime_error("");
}
