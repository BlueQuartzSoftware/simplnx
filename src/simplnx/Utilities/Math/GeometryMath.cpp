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

BoundingBox3Df nx::core::GeometryMath::FindBoundingBoxOfFace(const TriangleGeom& faces, int32 faceId)
{
  std::array<Point3Df, 3> points;
  faces.getFaceCoordinates(faceId, points);

  Point3Df ll = points[0];
  Point3Df ur = points[0];

  if(points[1][0] < ll[0])
  {
    ll[0] = points[1][0];
  }
  if(points[1][0] > ur[0])
  {
    ur[0] = points[1][0];
  }
  if(points[1][1] < ll[1])
  {
    ll[1] = points[1][1];
  }
  if(points[1][1] > ur[1])
  {
    ur[1] = points[1][1];
  }
  if(points[1][2] < ll[2])
  {
    ll[2] = points[1][2];
  }
  if(points[1][2] > ur[2])
  {
    ur[2] = points[1][2];
  }
  if(points[2][0] < ll[0])
  {
    ll[0] = points[2][0];
  }
  if(points[2][0] > ur[0])
  {
    ur[0] = points[2][0];
  }
  if(points[2][1] < ll[1])
  {
    ll[1] = points[2][1];
  }
  if(points[2][1] > ur[1])
  {
    ur[1] = points[2][1];
  }
  if(points[2][2] < ll[2])
  {
    ll[2] = points[2][2];
  }
  if(points[2][2] > ur[2])
  {
    ur[2] = points[2][2];
  }

  return {ll, ur};
}

BoundingBox3Df nx::core::GeometryMath::FindBoundingBoxOfFaces(const TriangleGeom& faces, const std::vector<int32>& faceIds)
{
  Point3Df ll(0, 0, 0);
  Point3Df ur(0, 0, 0);

  if(faceIds.empty())
  {
    return {ll, ur};
  }

  for(const auto& id : faceIds)
  {
    auto bounds = FindBoundingBoxOfFace(faces, id);
    Point3Df min = bounds.getMinPoint();
    Point3Df max = bounds.getMaxPoint();

    if(min[0] < ll[0])
    {
      ll[0] = min[0];
    }
    if(min[1] < ll[1])
    {
      ll[1] = min[1];
    }
    if(min[2] < ll[2])
    {
      ll[2] = min[2];
    }
    if(max[0] > ur[0])
    {
      ur[0] = max[0];
    }
    if(max[1] > ur[1])
    {
      ur[1] = max[1];
    }
    if(max[2] > ur[2])
    {
      ur[2] = max[2];
    }
  }

  return {ll, ur};
}

BoundingBox3Df nx::core::GeometryMath::FindBoundingBoxOfRotatedFace(TriangleGeom& faces, int32 faceId, float32 g[3][3])
{
  throw std::runtime_error("");
}
