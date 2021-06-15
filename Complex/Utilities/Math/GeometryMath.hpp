#pragma once

#include <vector>

#include "Complex/Common/BoundingBox.hpp"
#include "Complex/Common/EulerAngle.h"
#include "Complex/Common/Point2D.hpp"
#include "Complex/Common/Point3D.hpp"
#include "Complex/Common/Ray.hpp"

namespace Complex
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
 * @return float
 */
template <typename T>
T CosThetaBetweenVectors(const Complex::Point3D<T>& a, const Complex::Point3D<T>& b)
{
  throw std::exception();
}

/**
 * @brief Returns the angle between two vectors are each defined by a ZXZEuler.
 * @param a
 * @param b
 * @return float
 */
template <typename T>
T AngleBetweenVectors(const Complex::ZXZEuler<T>& a, const Complex::ZXZEuler<T>& b)
{
  throw std::exception();
}

/**
 * @brief Returns the distance between two points.
 * @param a
 * @param b
 * @return T
 */
template <typename T>
T FindDistanceBetweenPoints(const Complex::Point3D<T>& a, const Complex::Point3D<T>& b)
{
  throw std::exception();
}

/**
 * @brief Returns the area within a triangle defined by three points.
 * @param a
 * @param b
 * @param c
 * @return T
 */
template <typename T>
T FindTriangleArea(const Complex::Point3D<T>& a, const Complex::Point3D<T>& b, const Complex::Point3D<T>& c)
{
  throw std::exception();
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
T FindTetrahedronVolume(const Complex::Point3D<T>& p0, const Complex::Point3D<T>& p1, const Complex::Point3D<T>& p2, const Complex::Point3D<T>& p3)
{
  throw std::exception();
}

/**
 * @brief Returns the normal angle for an arbitrary polygon defined by an array
 * of vertices.
 * @param vertices
 * @param numVerts
 * @return Complex::ZXZEuler<float>
 */
ZXZEuler<float> FindPolygonNormal(const float* vertices, uint64_t numVerts)
{
  throw std::exception();
}

/**
 * @brief Returns the normal vector for a plane defined by three points along
 * its surface.
 * @param p0
 * @param p1
 * @param p2
 * @return Complex::ZXZEuler<T>
 */
template <typename T>
Complex::ZXZEuler<T> FindPlaneNormalVector(const Complex::Point3D<T>& p0, const Complex::Point3D<T>& p1, const Complex::Point3D<T>& p2)
{
  throw std::exception();
}

/**
 * @brief Finds the coefficients and normal for a plane defined by three points
 * along its surface.
 * @param p0
 * @param p1
 * @param p2
 * @param c
 * @param normal
 */
template <typename T>
void FindPlaneCoefficients(const Complex::Point3D<T>& p0, const Complex::Point3D<T>& p1, const Complex::Point3D<T>& p2, const float& c, ZXZEuler<float>& normal)
{
  throw std::exception();
}

/**
 * @brief Returns the distance between a point and the center of a triangle
 * defined by points along its corners.
 * @param p0
 * @param p1
 * @param p2
 * @param point
 * @return float
 */
template <typename T>
float FindDistanceToTriangleCentroid(const Complex::Point3D<T>& p0, const Complex::Point3D<T>& p1, const Complex::Point3D<T>& p2, const Complex::Point3D<T>& point)
{
  throw std::exception();
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
float FindDistanceFromPlane(const Complex::Point3D<T>& p0, const Complex::Point3D<T>& p1, const Complex::Point3D<T>& p2, const Complex::Point3D<T>& pos)
{
  throw std::exception();
}

/**
 * @brief Returns true if a point is within the specified box.
 * @param point
 * @param box
 * @return bool
 */
template <typename T>
bool IsPointInBox(const Complex::Point3D<T>& point, const Complex::BoundingBox<T>& box)
{
  throw std::exception();
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
// bool IsPointInPolyhedron(Complex::TriangleGeom* faces, Complex::Int32Int32DynamicListArray::ElementList* faceIds, Complex::VertexGeom* vertices, const Point3D<T>& point,
//                         const Complex::BoundingBox<float>& bounds, float radius, float& distToBoundary)
//{
// throw std::exception();
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
bool IsPointInTriangle3D(const Complex::Point3D<T>& p0, const Complex::Point3D<T>& p1, const Complex::Point3D<T>& p2, const Complex::Point3D<T>& point)
{
  throw std::exception();
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
bool IsPointInTriangle2D(const Complex::Point2D<T>& p0, const Complex::Point2D<T>& p1, const Complex::Point2D<T>& p2, const Complex::Point2D<T>& point)
{
  throw std::exception();
}

/**
 * @brief Returns true if a ray intersects the specified box. Returns false
 * otherwise.
 * @param ray
 * @param bounds
 * @return bool
 */
template <typename T>
bool DoesRayIntersectBox(Complex::Ray<T> ray, const Complex::BoundingBox<T>& bounds)
{
  throw std::exception();
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
uint8_t FindRayIntersectionsWithSphere(const Complex::Ray<T>& ray, const Complex::Point3D<T>& origin, T radius, std::vector<Point3D<T>>& intersections)
{
  throw std::exception();
}

/**
 * @brief Returns the length of an array that falls within the specified box.
 * @param ray
 * @param box
 * @return float
 */
template <typename T>
T GetLengthOfRayInBox(const Complex::Ray<T>& ray, const Complex::BoundingBox<T>& box)
{
  throw std::exception();
}

/**
 * @brief Generates a random Ray with the specified length.
 * @param length
 * @return Complex::Ray<float>
 */
Complex::Ray<float> GenerateRandomRay(float length)
{
  throw std::exception();
}

/**
 * @brief Returns the BoundingBox around the specified vertices.
 * @param verts
 * @return Complex::BoundingBox<float>
 */
Complex::BoundingBox<float> FindBoundingBoxOfVertices(Complex::VertexGeom* verts)
{
  throw std::exception();
}

/**
 * @brief Returns the BoundingBox around the specified face.
 * @param faces
 * @param faceId
 * @return Complex::BoundingBox<float>
 */
Complex::BoundingBox<float> FindBoundingBoxOfFace(Complex::TriangleGeom* faces, int32_t faceId)
{
  throw std::exception();
}

/**
 * @brief Returns the BoundingBox around the specified face manipulated by the
 * provided rotation matrix.
 * @param faces
 * @param faceId
 * @param float[3][3]
 * @return Complex::BoundingBox<float>
 */
Complex::BoundingBox<float> FindBoundingBoxOfRotatedFace(Complex::TriangleGeom* faces, int32_t faceId, float g[3][3])
{
  throw std::exception();
}

/**
 * @param TriangleGeom* faces
 * @param Int32Int32DynamicListArray.ElementList faceIds
 * @return Complex::BoundingBox<float>
 */
// Complex::BoundingBox<float> FindBoundingBoxOfFaces(Complex::TriangleGeom* faces, const Int32Int32DynamicListArray.ElementList& faceIds)
//{
//  throw std::exception();
//}

/**
 * @brief Returns the bounding box around the specified faces manipulated by the
 * provided rotation matrix.
 * @param faces
 * @param faceIds
 * @param float g[3][3]
 * @return Complex::BoundingBox<float>
 */
// Complex::BoundingBox<float> FindBoundingBoxOfRotatedFaces(TriangleGeom* faces, const Int32Int32DynamicListArray.ElementList& faceIds, float g[3][3])
//{
//  throw std::exception();
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
uint8_t RayIntersectsTriangle(const Ray<T>& ray, const Complex::Point3D<T>& p0, const Complex::Point3D<T>& p1, const Complex::Point3D<T>& p2, std::vector<Point3D<T>>& inter)
{
  throw std::exception();
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
  throw std::exception();
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
  throw std::exception();
}
} // namespace GeometryMath
} // namespace Complex
