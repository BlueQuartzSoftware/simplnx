#include "GeometryMath.hpp"

#include <stdexcept>

using namespace complex;

float complex::GeometryMath::AngleBetweenVectors(const complex::ZXZEuler& a, const complex::ZXZEuler& b)
{
  throw std::runtime_error("");
}

ZXZEuler complex::GeometryMath::FindPolygonNormal(const float* vertices, uint64_t numVerts)
{
  throw std::runtime_error("");
}

complex::ZXZEuler complex::GeometryMath::FindPlaneNormalVector(const complex::Point3D<float>& p0, const complex::Point3D<float>& p1, const complex::Point3D<float>& p2)
{
  throw std::runtime_error("");
}

complex::Ray<float> complex::GeometryMath::GenerateRandomRay(float length)
{
  throw std::runtime_error("");
}

complex::BoundingBox<float> complex::GeometryMath::FindBoundingBoxOfVertices(complex::VertexGeom* verts)
{
  throw std::runtime_error("");
}

complex::BoundingBox<float> complex::GeometryMath::FindBoundingBoxOfFace(complex::TriangleGeom* faces, int32_t faceId)
{
  throw std::runtime_error("");
}

complex::BoundingBox<float> complex::GeometryMath::FindBoundingBoxOfRotatedFace(complex::TriangleGeom* faces, int32_t faceId, float g[3][3])
{
  throw std::runtime_error("");
}
