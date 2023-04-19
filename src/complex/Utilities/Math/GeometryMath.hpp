#pragma once

#include "complex/Common/Array.hpp"
#include "complex/Common/BoundingBox.hpp"
#include "complex/Common/EulerAngle.hpp"
#include "complex/Common/Ray.hpp"
#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

#include <stdexcept>
#include <vector>

namespace complex
{
class TriangleGeom;
class VertexGeom;

namespace GeometryMath
{
/**
 * @brief Returns the cosine between two angles defined by a point along each
 * vector. The vectors are assumed to cross at (0,0,0).
 * @param a
 * @param b
 * @return T
 */
template <typename T>
inline T CosThetaBetweenVectors(const complex::Point3D<T>& a, const complex::Point3D<T>& b)
{
  T norm1 = sqrt(a.sumOfSquares());
  T norm2 = sqrt(b.sumOfSquares());
  if(norm1 == 0 || norm2 == 0)
  {
    return 1.0;
  }
  return (a.dot(b)) / (norm1 * norm2);
}

/**
 * @brief Returns the angle between two vectors that are defined by a ZXZEuler.
 * @param a
 * @param b
 * @return float
 */
float32 COMPLEX_EXPORT AngleBetweenVectors(const complex::ZXZEuler& a, const complex::ZXZEuler& b);

/**
 * @brief Returns the distance between two points.
 * @param a
 * @param b
 * @return T
 */
template <typename T>
inline T FindDistanceBetweenPoints(const complex::Point3D<T>& a, const complex::Point3D<T>& b)
{
  return sqrt((b - a).sumOfSquares());
}

/**
 * @brief Returns the distance between two points.
 * @param a
 * @param b
 * @return T
 */
template <typename T>
inline T FindDistanceBetweenPoints(const complex::Point2D<T>& a, const complex::Point2D<T>& b)
{
  return sqrt((b - a).sumOfSquares());
}

/**
 * @brief Returns the area within a triangle defined by three points.
 * @param a
 * @param b
 * @param c
 * @return T
 */
template <typename T>
inline T FindTriangleArea(const complex::Point3D<T>& a, const complex::Point3D<T>& b, const complex::Point3D<T>& c)
{
  return ((b[0] - a[0]) * (c[1] - a[1])) - ((c[0] - a[0]) * (b[1] - a[1]));
}

/**
 * @brief Returns the volume within a tetrahedron defined by four points.
 * @param p0
 * @param p1
 * @param p2
 * @param p3
 * @return float
 */
template <typename T>
T FindTetrahedronVolume(const complex::Point3D<T>& p0, const complex::Point3D<T>& p1, const complex::Point3D<T>& p2, const complex::Point3D<T>& p3)
{
  throw std::runtime_error("");
}

/**
 * @brief Returns the normal angle for an arbitrary polygon defined by an array
 * of vertices.
 * @param vertices
 * @param numVerts
 * @return complex::ZXZEuler<float32>
 */
ZXZEuler COMPLEX_EXPORT FindPolygonNormal(const float* vertices, uint64 numVerts);

/**
 * @brief Returns the normal vector for a plane defined by three points along
 * its surface.
 * @param p0
 * @param p1
 * @param p2
 * @return complex::Vec3<T>
 */
 template<typename T>
inline complex::Vec3<T> FindPlaneNormalVector(const complex::Point3D<T>& p0, const complex::Point3D<T>& p1, const complex::Point3D<T>& p2)
{
  return (p1 - p0).cross(p2 - p0);
}

/**
 * @brief Finds the coefficients for a plane defined by three points
 * along its surface.
 * @param p0
 * @param normal
 * @return T - the coefficients for the plane
 */
template <typename T>
inline T FindPlaneCoefficients(const complex::Point3D<T>& p0, const complex::Vec3<T>& normal)
{
  return p0.dot(normal);
}

/**
 * @brief Returns the distance between a point and the center of a triangle
 * defined by points along its corners.
 * @param p0
 * @param p1
 * @param p2
 * @param point
 * @return T
 */
template <typename T>
T FindDistanceToTriangleCentroid(const complex::Point3D<T>& p0, const complex::Point3D<T>& p1, const complex::Point3D<T>& p2, const complex::Point3D<T>& point)
{
  return FindDistanceBetweenPoints((p0 + p1 + p2)/static_cast<T>(3.0), point);
}

/**
 * @brief Returns the distance between a point and a plane defined by three
 * points along its surface.
 * @param p0
 * @param p1
 * @param p2
 * @param pos
 * @return float
 */
template <typename T>
float32 FindDistanceFromPlane(const complex::Point3D<T>& p0, const complex::Point3D<T>& p1, const complex::Point3D<T>& p2, const complex::Point3D<T>& pos)
{
  throw std::runtime_error("");
}

/**
 * @brief Returns true if a point is within the specified box.
 * @param point
 * @param box
 * @return bool
 */
template <typename T>
inline bool IsPointInBox(const complex::Point3D<T>& point, const complex::BoundingBox3D<T>& box)
{
  auto min = box.getMinPoint();
  auto max = box.getMaxPoint();
  return (min[0] <= point[0]) && (point[0] <= max[0]) && (min[1] <= point[1]) && (point[1] <= max[1]) && (min[2] <= point[2]) && (point[2] <= max[2]);
}

/**
 * @param faces
 * @param faceIds
 * @param vertices
 * @param point
 * @param bounds
 * @param radius
 * @param distToBoundary
 * @return bool
 */
// template <typename T>
// bool IsPointInPolyhedron(complex::TriangleGeom* faces, complex::Int32Int32DynamicListArray::ElementList* faceIds, complex::VertexGeom* vertices, const Point3D<T>& point,
//                         const complex::BoundingBox<float32>& bounds, float32 radius, float& distToBoundary)
//{
// throw std::runtime_error("");
//}

/**
 * @brief Returns true if a point is within the triangle defined by three
 * specified points. Returns false otherwise. This function operates in 3D
 * space.
 * @param p0
 * @param p1
 * @param p2
 * @param point
 * @return bool
 */
template <typename T>
bool IsPointInTriangle3D(const complex::Point3D<T>& p0, const complex::Point3D<T>& p1, const complex::Point3D<T>& p2, const complex::Point3D<T>& point)
{
  throw std::runtime_error("");
}

/**
 * @brief Returns true if a point is within the triangle defined by three
 * specified points. Returns false otherwise. This function operates in 2D
 * space.
 * @param p0
 * @param p1
 * @param p2
 * @param point
 * @return bool
 */
template <typename T>
bool IsPointInTriangle2D(const complex::Point2D<T>& p0, const complex::Point2D<T>& p1, const complex::Point2D<T>& p2, const complex::Point2D<T>& point)
{
  throw std::runtime_error("");
}

/**
 * @brief Returns true if a ray intersects the specified box. Returns false
 * otherwise.
 * @param ray
 * @param bounds
 * @return bool
 */
template <typename T>
bool DoesRayIntersectBox(complex::Ray<T> ray, const complex::BoundingBox3D<T>& bounds)
{
  throw std::runtime_error("");
}

/**
 * @brief Finds intersections between a ray and sphere defined by a center
 * point and radius. The intersections parameter is updated to reflect the
 * points where intersections are found. Returns the number of points at
 * which the Ray intersects the sphere.
 * @param ray
 * @param origin
 * @param radius
 * @param intersections
 * @return bool
 */
template <typename T>
uint8 FindRayIntersectionsWithSphere(const complex::Ray<T>& ray, const complex::Point3D<T>& origin, T radius, std::vector<Point3D<T>>& intersections)
{
  throw std::runtime_error("");
}

/**
 * @brief Returns the length of an array that falls within the specified box.
 * @param ray
 * @param box
 * @return float
 */
template <typename T>
T GetLengthOfRayInBox(const complex::Ray<T>& ray, const complex::BoundingBox3D<T>& box)
{
  throw std::runtime_error("");
}

/**
 * @brief Generates a random Ray with the specified length.
 * @param length
 * @return complex::Ray<float32>
 */
complex::Ray<float32> COMPLEX_EXPORT GenerateRandomRay(float32 length);

/**
 * @brief Returns the BoundingBox around the specified vertices.
 * @param verts
 * @return complex::BoundingBox<float32>
 */
complex::BoundingBox3Df COMPLEX_EXPORT FindBoundingBoxOfVertices(complex::VertexGeom* verts);

/**
 * @brief Returns the BoundingBox around the specified face.
 * @param faces
 * @param faceId
 * @return complex::BoundingBox<float32>
 */
complex::BoundingBox3Df COMPLEX_EXPORT FindBoundingBoxOfFace(complex::TriangleGeom* faces, int32 faceId);

/**
 * @brief Returns the BoundingBox around the specified face manipulated by the
 * provided rotation matrix.
 * @param faces
 * @param faceId
 * @param float[3][3]
 * @return complex::BoundingBox<float32>
 */
complex::BoundingBox3Df COMPLEX_EXPORT FindBoundingBoxOfRotatedFace(complex::TriangleGeom* faces, int32 faceId, float32 g[3][3]);

/**
 * @param TriangleGeom* faces
 * @param Int32Int32DynamicListArray.ElementList faceIds
 * @return complex::BoundingBox<float32>
 */
//complex::BoundingBox3Df FindBoundingBoxOfFaces(complex::TriangleGeom* faces, const Int32Int32DynamicListArray.ElementList& faceIds)
//{
//  throw std::runtime_error("");
//}

/**
 * @brief Returns the bounding box around the specified faces manipulated by the
 * provided rotation matrix.
 * @param faces
 * @param faceIds
 * @param float32 g[3][3]
 * @return complex::BoundingBox<float32>
 */
// complex::BoundingBox<float32> FindBoundingBoxOfRotatedFaces(TriangleGeom* faces, const Int32Int32DynamicListArray.ElementList& faceIds, float32 g[3][3])
//{
//  throw std::runtime_error("");
//}

/**
 * @brief Checks if the specified Ray intersects a triangle defined by the
 * corner points. The inter parameter is updated to reflect the points at which
 * the ray intersects the triangle. Returns the number of points at which the
 * Ray intersects the triangle.
 * @param ray
 * @param p0
 * @param p1
 * @param p2
 * @param inter
 * @return char
 */
template <typename T>
uint8 RayIntersectsTriangle(const Ray<T>& ray, const complex::Point3D<T>& p0, const complex::Point3D<T>& p1, const complex::Point3D<T>& p2, std::vector<Point3D<T>>& inter)
{
  throw std::runtime_error("");
}

/**
 * @brief Returns true if the specified Ray crosses into or out of the triangle
 * defined by its corner points. Returns false otherwise.
 *
 * Tangental intersections where the Ray touches the triangle but does not
 * enter or leave will return false.
 * @param ray
 * @param p0
 * @param p1
 * @param p2
 * @return bool
 */
template <typename T>
bool RayCrossesTriangle(const Ray<T>& ray, const Point3D<T>& p0, const Point3D<T>& p1, const Point3D<T>& p2)
{
  throw std::runtime_error("");
}

/**
 * @brief Returns true if the specified Ray intersects a plan defined by three
 * points along the surface. Returns false otherwise.
 * @param ray
 * @param p0
 * @param p1
 * @param p2
 * @return bool
 */
template <typename T>
bool RayIntersectsPlane(const Ray<T>& ray, const Point3D<T>& p0, const Point3D<T>& p1, const Point3D<T>& p2)
{
  throw std::runtime_error("");
}
} // namespace GeometryMath
} // namespace complex
