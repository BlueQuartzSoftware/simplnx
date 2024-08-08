#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/BoundingBox.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/EulerAngle.hpp"
#include "simplnx/Common/Ray.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/simplnx_export.hpp"

#include <chrono>
#include <functional>
#include <random>
#include <stdexcept>
#include <vector>

namespace nx::core
{
class TriangleGeom;
class VertexGeom;

namespace GeometryMath
{
namespace detail
{
struct GeometryStoreCache
{
  GeometryStoreCache(const AbstractDataStore<float32>& verticesStore, const AbstractDataStore<IGeometry::MeshIndexType>& facesStore, usize numVertsPerFace)
  : VerticesStoreRef(verticesStore)
  , FacesStoreRef(facesStore)
  , NumVertsPerFace(numVertsPerFace)
  {
  }

  const AbstractDataStore<float32>& VerticesStoreRef;
  const AbstractDataStore<IGeometry::MeshIndexType>& FacesStoreRef;
  usize NumVertsPerFace;
};

template <typename T>
concept FloatType = std::is_floating_point_v<T>;

template <FloatType FloatT>
std::array<Point3D<FloatT>, 3> GetFaceCoordinates(const GeometryStoreCache& cache, usize faceId)
{
  std::array<Point3D<FloatT>, 3> points;
  std::vector<usize> verts(cache.NumVertsPerFace);
  {
    const usize offset = faceId * cache.NumVertsPerFace;
    if(offset + cache.NumVertsPerFace <= cache.FacesStoreRef.getSize())
    {
      for(usize i = 0; i < cache.NumVertsPerFace; i++)
      {
        verts[i] = cache.FacesStoreRef.at(offset + i);
      }
    }
  }
  for(usize index = 0; index < verts.size(); index++)
  {
    const usize offset = verts[index] * 3;
    for(usize i = 0; i < 3; i++)
    {
      points[index][i] = static_cast<FloatT>(cache.VerticesStoreRef.at(offset + i));
    }
  }
  return points;
}
} // namespace detail
/**
 * @brief Returns the cosine between two angles defined by a point along each
 * vector. The vectors are assumed to cross at (0,0,0).
 * @param a
 * @param b
 * @return T
 */
template <typename T>
inline T CosThetaBetweenVectors(const nx::core::Point3D<T>& a, const nx::core::Point3D<T>& b)
{
  T norm1 = a.magnitude();
  T norm2 = b.magnitude();
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
float32 SIMPLNX_EXPORT AngleBetweenVectors(const nx::core::ZXZEuler& a, const nx::core::ZXZEuler& b);

/**
 * @brief Returns the distance between two points.
 * @param a
 * @param b
 * @return T
 */
template <typename T>
inline T FindDistanceBetweenPoints(const nx::core::Point3D<T>& a, const nx::core::Point3D<T>& b)
{
  return std::sqrt((b - a).sumOfSquares());
}

/**
 * @brief Returns the distance between two points.
 * @param a
 * @param b
 * @return T
 */
template <typename T>
inline T FindDistanceBetweenPoints(const nx::core::Point2D<T>& a, const nx::core::Point2D<T>& b)
{
  return std::sqrt((b - a).sumOfSquares());
}

/**
 * @brief Returns the area within a triangle defined by three points.
 * @param a
 * @param b
 * @param c
 * @return T
 */
template <typename T>
inline T FindTriangleArea(const nx::core::Point3D<T>& a, const nx::core::Point3D<T>& b, const nx::core::Point3D<T>& c)
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
T FindTetrahedronVolume(const nx::core::Point3D<T>& p0, const nx::core::Point3D<T>& p1, const nx::core::Point3D<T>& p2, const nx::core::Point3D<T>& p3)
{
  auto diffA = p0 - p3;
  auto diffB = p1 - p3;
  auto diffC = p2 - p3;
  return (diffA[2] * ((diffB[0] * diffC[1]) - (diffB[1] * diffC[0]))) + (diffA[1] * ((diffB[2] * diffC[0]) - (diffB[0] * diffC[2]))) + (diffA[0] * ((diffB[1] * diffC[2]) - (diffB[2] * diffC[1])));
}

/**
 * @brief Returns the normal vector for a plane defined by three points along
 * its surface.
 * @param p0
 * @param p1
 * @param p2
 * @return nx::core::Vec3<T>
 */
template <typename T>
inline nx::core::Vec3<T> FindPlaneNormalVector(const nx::core::Point3D<T>& p0, const nx::core::Point3D<T>& p1, const nx::core::Point3D<T>& p2)
{
  return (p1 - p0).cross(p2 - p0);
}

/**
 * @brief Returns the normal angle for an arbitrary polygon defined by an array
 * of vertices.
 * @param points must be greater than 2 in size
 * @return Point3D<T>
 */
template <typename T>
Point3D<T> FindPolygonNormal(nonstd::span<Point3D<T>> points)
{
  if(points.size() < 3)
  {
    throw std::runtime_error("This function cannot process under 3 points");
  }

  if(points.size() == 3)
  {
    return FindPlaneNormalVector(points[0], points[1], points[2]);
  }

  Point3D<T> normal = {0, 0, 0};
  for(usize i = 0; i < points.size() - 3; i++)
  {
    std::transform(normal.begin(), normal.end(), ((points[i + 2] - points[i + 1]).cross(points[i] - points[i + 1])).begin(), normal.begin(), std::plus<T>());
  }
  return normal;
}

/**
 * @brief Finds the coefficients for a plane defined by three points
 * along its surface.
 * @param p0
 * @param normal
 * @return T - the coefficients for the plane
 */
template <typename T>
inline T FindPlaneCoefficients(const nx::core::Point3D<T>& p0, const nx::core::Vec3<T>& normal)
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
T FindDistanceToTriangleCentroid(const nx::core::Point3D<T>& p0, const nx::core::Point3D<T>& p1, const nx::core::Point3D<T>& p2, const nx::core::Point3D<T>& point)
{
  return FindDistanceBetweenPoints((p0 + p1 + p2) / static_cast<T>(3.0), point);
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
T FindDistanceFromPlane(const nx::core::Point3D<T>& p0, const nx::core::Point3D<T>& p1, const nx::core::Point3D<T>& p2, const nx::core::Point3D<T>& pos)
{
  auto normal = FindPlaneNormalVector(p0, p1, p2);
  return (pos.dot(normal) - FindPlaneCoefficients(p0, normal)) /= sqrt(normal.sumOfSquares());
}

/**
 * @brief Returns true if a point is within the specified box.
 * @param point
 * @param box
 * @return bool
 */
template <typename T>
inline bool IsPointInBox(const nx::core::Point3D<T>& point, const nx::core::BoundingBox3D<T>& box)
{
  auto min = box.getMinPoint();
  auto max = box.getMaxPoint();
  return (min[0] <= point[0]) && (point[0] <= max[0]) && (min[1] <= point[1]) && (point[1] <= max[1]) && (min[2] <= point[2]) && (point[2] <= max[2]);
}

/**
 * @brief Returns true if a ray intersects the specified box. Returns false
 * otherwise.
 * @param ray
 * @param bounds
 * @return bool
 */
template <typename T>
bool DoesRayIntersectBox(nx::core::Ray<T> ray, const nx::core::BoundingBox3D<T>& bounds)
{
  auto origin = ray.getOrigin();
  auto end = ray.getEndPoint();
  auto min = bounds.getMinPoint();
  auto max = bounds.getMaxPoint();

  if((min[0] > origin[0]) && (min[0] > end[0]))
  {
    return false;
  }
  if((max[0] < origin[0]) && (max[0] < end[0]))
  {
    return false;
  }
  if((min[1] > origin[1]) && (min[1] > end[1]))
  {
    return false;
  }
  if((max[1] < origin[1]) && (max[1] < end[1]))
  {
    return false;
  }
  if((min[2] > origin[2]) && (min[2] > end[2]))
  {
    return false;
  }
  if((max[2] < origin[2]) && (max[2] < end[2]))
  {
    return false;
  }
  return true;
}

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
bool IsPointInTriangle3D(const nx::core::Point3D<T>& p0, const nx::core::Point3D<T>& p1, const nx::core::Point3D<T>& p2, const nx::core::Point3D<T>& point)
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
 * @return char
 */
template <typename T>
char IsPointInTriangle(const nx::core::Point3D<T>& p0, const nx::core::Point3D<T>& p1, const nx::core::Point3D<T>& p2, const nx::core::Point3D<T>& point)
{
  T area0 = FindTriangleArea(point, p0, p1);
  T area1 = FindTriangleArea(point, p1, p2);
  T area2 = FindTriangleArea(point, p2, p0);

  if((area0 == 0 && area1 > 0 && area2 > 0) || (area1 == 0 && area0 > 0 && area2 > 0) || (area2 == 0 && area0 > 0 && area1 > 0))
  {
    return 'E';
  }
  if((area0 == 0 && area1 < 0 && area2 < 0) || (area1 == 0 && area0 < 0 && area2 < 0) || (area2 == 0 && area0 < 0 && area1 < 0))
  {
    return 'E';
  }
  if((area0 > 0 && area1 > 0 && area2 > 0) || (area0 < 0 && area1 < 0 && area2 < 0))
  {
    return 'F';
  }
  if((area0 == 0 && area1 == 0 && area2 == 0))
  {
    return '?';
  }
  else if((area0 == 0 && area1 == 0) || (area0 == 0 && area2 == 0) || (area1 == 0 && area2 == 0))
  {
    return 'V';
  }
  else
  {
    return '0';
  }
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
uint8 FindRayIntersectionsWithSphere(const nx::core::Ray<T>& ray, const nx::core::Point3D<T>& origin, T radius, std::vector<Point3D<T>>& intersections)
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
T GetLengthOfRayInBox(const nx::core::Ray<T>& ray, const nx::core::BoundingBox3D<T>& box)
{
  throw std::runtime_error("");
}

/**
 * @brief Returns the BoundingBox around the specified vertices.
 * @param verts
 * @return nx::core::BoundingBox<float32>
 */
nx::core::BoundingBox3Df SIMPLNX_EXPORT FindBoundingBoxOfVertices(nx::core::INodeGeometry0D& geom);

/**
 * @brief Returns the BoundingBox around the specified face manipulated by the
 * provided rotation matrix.
 * @param faces
 * @param faceId
 * @param float[3][3]
 * @return nx::core::BoundingBox<float32>
 */
nx::core::BoundingBox3Df SIMPLNX_EXPORT FindBoundingBoxOfRotatedFace(nx::core::TriangleGeom& faces, int32 faceId, float32 g[3][3]);

/**
 * @brief Returns the BoundingBox around the specified face.
 * @param faces
 * @param faceId
 * @return nx::core::BoundingBox<float32>
 */
nx::core::BoundingBox3Df FindBoundingBoxOfFace(const nx::core::TriangleGeom& faces, int32 faceId);

/**
 * @brief Returns the BoundingBox around the specified face.
 * @param faces
 * @param faceId
 * @return nx::core::BoundingBox<float32>
 */
nx::core::BoundingBox3Df FindBoundingBoxOfFace(const detail::GeometryStoreCache& cache, const nx::core::TriangleGeom& triangleGeom, int32 faceId);

/**
 * @param TriangleGeom* faces
 * @param Int32Int32DynamicListArray.ElementList faceIds
 * @return nx::core::BoundingBox<float32>
 */
nx::core::BoundingBox3Df SIMPLNX_EXPORT FindBoundingBoxOfFaces(const nx::core::TriangleGeom& triangleGeom, const std::vector<int32>& faceIds);

/**
 * @brief Returns the bounding box around the specified faces manipulated by the
 * provided rotation matrix.
 * @param faces
 * @param faceIds
 * @param float32 g[3][3]
 * @return nx::core::BoundingBox<float32>
 */
// nx::core::BoundingBox<float32> FindBoundingBoxOfRotatedFaces(TriangleGeom* faces, const Int32Int32DynamicListArray.ElementList& faceIds, float32 g[3][3])
//{
//  throw std::runtime_error("");
//}

/**
 * @brief Returns true if the specified Ray crosses into or out of the triangle
 * defined by its corner points. Returns false otherwise.
 *
 * Tangential intersections where the Ray touches the triangle but does not
 * enter or leave will return false.
 * @param ray
 * @param p0
 * @param p1
 * @param p2
 * @return bool
 */
template <typename T>
char RayCrossesTriangle(const Point3D<T>& p0, const Point3D<T>& p1, const Point3D<T>& p2, const Ray<T>& ray)
{
  T vol0 = FindTetrahedronVolume(ray.getOrigin(), p0, p1, ray.getEndPoint());
  T vol1 = FindTetrahedronVolume(ray.getOrigin(), p1, p2, ray.getEndPoint());
  T vol2 = FindTetrahedronVolume(ray.getOrigin(), p2, p0, ray.getEndPoint());

  if((vol0 > 0 && vol1 > 0 && vol2 > 0) || (vol0 < 0 && vol1 < 0 && vol2 < 0))
  {
    return 'f';
  }
  if((vol0 > 0 || vol1 > 0 || vol2 > 0) && (vol0 < 0 || vol1 < 0 || vol2 < 0))
  {
    return '0';
  }
  if((vol0 == 0 && vol1 == 0 && vol2 == 0))
  {
    return '?';
  }
  if((vol0 == 0 && vol1 == 0) || (vol0 == 0 && vol2 == 0) || (vol1 == 0 && vol2 == 0))
  {
    return 'v';
  }
  else if(vol0 == 0 || vol1 == 0 || vol2 == 0)
  {
    return 'e';
  }
  else
  {
    return '?';
  }
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
char RayIntersectsPlane(const Ray<T>& ray, const Point3D<T>& p0, const Point3D<T>& p1, const Point3D<T>& p2)
{
  Vec3<T> normal = FindPlaneNormalVector(p0, p1, p2);
  T d = FindPlaneCoefficients(p0, normal);

  T numerator = d - ray.getOrigin().dot(normal);

  Point3D<T> rq = ray.getEndPoint() - ray.getOrigin();
  T denominator = rq.dot(normal);

  if(denominator == 0.0)
  {
    if(numerator == 0.0)
    {
      return 'p';
    }

    return '0';
  }

  T t = numerator / denominator;
  if(t > 0.0 && t < 1.0)
  {
    return '1';
  }
  if(numerator == 0.0)
  {
    return 'q';
  }
  if(numerator == denominator)
  {
    return 'r';
  }

  return '0';
}

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
char RayIntersectsTriangle(const Ray<T>& ray, const nx::core::Point3D<T>& p0, const nx::core::Point3D<T>& p1, const nx::core::Point3D<T>& p2)
{
  char code = RayIntersectsPlane(ray, p0, p1, p2);

  if(code == '0')
  {
    return '0';
  }
  if(code == 'q')
  {
    return IsPointInTriangle(p0, p1, p2, ray.getOrigin());
  }
  if(code == 'r')
  {
    return IsPointInTriangle(p0, p1, p2, ray.getEndPoint());
  }
  if(code == 'p')
  {
    return 'p';
  }
  else if(code == '1')
  {
    return RayCrossesTriangle(p0, p1, p2, ray);
  }
  else
  {
    return code;
  }
}

/**
 * @breif !!!Uses unseeded randomness, but only for validity checks [SHOULD not affect outcomes]!!! Determines if a point is in the polyhedron
 * @param faces the geometry to query
 * @param faceIds the list of feature ids
 * @param faceBBs the bounding boxes of each id in the geometry
 * @param point search point
 * @param bounds overarching bounding box for all of the geometry
 * @param radius length of ray
 * distToBoundary param can be added down the line (subtract intersect point from origin)
 * @return bool
 */
template <typename T>
char IsPointInPolyhedron(const nx::core::TriangleGeom& triangleGeomRef, const std::vector<int32>& faceIds, const std::vector<BoundingBox3D<T>>& faceBBs, const Point3D<T>& point,
                         const nx::core::BoundingBox3D<T>& bounds, T radius)
{
  usize iter = 0, crossings = 0;

  //* If query point is outside bounding box, finished. */
  if(!IsPointInBox(point, bounds))
  {
    return 'o';
  }

  // Standard mersenne_twister_engine random seed
  // std::mt19937_64 generator(static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::mt19937_64 generator(std::mt19937_64::default_seed);
  std::uniform_real_distribution<T> distribution(0.0, 1.0);

  detail::GeometryStoreCache cache(triangleGeomRef.getVertices()->getDataStoreRef(), triangleGeomRef.getFaces()->getDataStoreRef(), triangleGeomRef.getNumberOfVerticesPerFace());

  usize numFaces = faceIds.size();
  while(iter++ < numFaces)
  {
    crossings = 0;

    std::array<T, 3> eulerAngles;

    float rand1 = distribution(generator);
    float rand2 = distribution(generator);

    eulerAngles[2] = (2.0f * rand1) - 1.0f;
    float t = Constants::k_2PiF * rand2;
    float w = std::sqrt(1.0f - (eulerAngles[2] * eulerAngles[2]));
    eulerAngles[0] = w * std::cos(t);
    eulerAngles[1] = w * std::sin(t);

    // Generate and add ray to point to find other end
    Ray<T> ray(point, ZXZEuler(eulerAngles.data()), radius);

    bool doNextCheck = false;
    for(usize face = 0; face < numFaces; face++)
    {
      char code = '?';
      if(!DoesRayIntersectBox(ray, faceBBs[faceIds[face]]))
      {
        code = '0';
      }
      else
      {
        std::array<Point3D<T>, 3> coords = detail::GetFaceCoordinates<T>(cache, faceIds[face]);
        code = RayIntersectsTriangle(ray, coords[0], coords[1], coords[2]);
      }

      /* If ray is degenerate, then goto outer while to generate another. */
      if(code == 'p' || code == 'v' || code == 'e' || code == '?')
      {
        doNextCheck = true;
        break;
      }

      /* If ray hits face at interior point, increment crossings. */
      else if(code == 'f')
      {
        crossings++;
      }

      /* If query endpoint q sits on a V/E/F, return that code. */
      else if(code == 'V' || code == 'E' || code == 'F')
      {
        return (code);
      }

    } /* End check each face */
    if(doNextCheck)
    {
      continue;
    }
    /* No degeneracies encountered: ray is generic, so finished. */
    break;

  } /* End while loop */

  /* q strictly interior to polyhedron if an odd number of crossings. */
  if((crossings % 2) == 1)
  {
    return 'i';
  }

  return 'o';
}
} // namespace GeometryMath
} // namespace nx::core
