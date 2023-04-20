#include "GeometryMath.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"

#include <stdexcept>

using namespace complex;

float32 complex::GeometryMath::AngleBetweenVectors(const complex::ZXZEuler& a, const complex::ZXZEuler& b)
{
  float norm1 = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
  float norm2 = sqrtf(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
  float cosAng = (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / (norm1 * norm2);
  if(cosAng < -1.0f)
  {
    cosAng = -1.0f;
  }
  else if(cosAng > 1.0)
  {
    cosAng = 1.0;
  }
  return acosf(cosAng);
}

Ray<float32> complex::GeometryMath::GenerateRandomRay(float32 length)
{
  throw std::runtime_error("");
}

BoundingBox3Df complex::GeometryMath::FindBoundingBoxOfVertices(INodeGeometry0D& geom)
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

  return {ll, ur}; //should be valid
}

BoundingBox3Df complex::GeometryMath::FindBoundingBoxOfFace(TriangleGeom& faces, int32 faceId)
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

BoundingBox3Df complex::GeometryMath::FindBoundingBoxOfRotatedFace(TriangleGeom* faces, int32 faceId, float32 g[3][3])
{
  throw std::runtime_error("");
}
