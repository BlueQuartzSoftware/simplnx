#include "GeometryMath.hpp"

using namespace complex;

float complex::GeometryMath::AngleBetweenVectors(const complex::ZXZEuler& a, const complex::ZXZEuler& b)
{
  throw std::exception();
}

ZXZEuler complex::GeometryMath::FindPolygonNormal(const float* vertices, uint64_t numVerts)
{
  throw std::exception();
}

complex::ZXZEuler complex::GeometryMath::FindPlaneNormalVector(const complex::Point3D<float>& p0, const complex::Point3D<float>& p1, const complex::Point3D<float>& p2)
{
  throw std::exception();
}

complex::Ray<float> complex::GeometryMath::GenerateRandomRay(float length)
{
  throw std::exception();
}

complex::BoundingBox<float> complex::GeometryMath::FindBoundingBoxOfVertices(complex::VertexGeom* verts)
{
  throw std::exception();
}

complex::BoundingBox<float> complex::GeometryMath::FindBoundingBoxOfFace(complex::TriangleGeom* faces, int32_t faceId)
{
  throw std::exception();
}

complex::BoundingBox<float> complex::GeometryMath::FindBoundingBoxOfRotatedFace(complex::TriangleGeom* faces, int32_t faceId, float g[3][3])
{
  throw std::exception();
}
