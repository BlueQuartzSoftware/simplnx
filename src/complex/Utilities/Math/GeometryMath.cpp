#include "GeometryMath.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"

#include <stdexcept>

using namespace complex;

float32 complex::GeometryMath::AngleBetweenVectors(const complex::ZXZEuler& a, const complex::ZXZEuler& b)
{
  throw std::runtime_error("");
}

ZXZEuler complex::GeometryMath::FindPolygonNormal(const float* vertices, uint64 numVerts)
{
  throw std::runtime_error("");
}

Ray<float32> complex::GeometryMath::GenerateRandomRay(float32 length)
{
  throw std::runtime_error("");
}

BoundingBox3Df complex::GeometryMath::FindBoundingBoxOfVertices(VertexGeom* verts)
{
  throw std::runtime_error("");
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
