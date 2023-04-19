#include "GeometryMath.hpp"

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

complex::Ray<float32> complex::GeometryMath::GenerateRandomRay(float32 length)
{
  throw std::runtime_error("");
}

complex::BoundingBox3Df complex::GeometryMath::FindBoundingBoxOfVertices(complex::VertexGeom* verts)
{
  throw std::runtime_error("");
}

complex::BoundingBox3Df complex::GeometryMath::FindBoundingBoxOfFace(complex::TriangleGeom* faces, int32 faceId)
{
  throw std::runtime_error("");
}

complex::BoundingBox3Df complex::GeometryMath::FindBoundingBoxOfRotatedFace(complex::TriangleGeom* faces, int32 faceId, float32 g[3][3])
{
  throw std::runtime_error("");
}
