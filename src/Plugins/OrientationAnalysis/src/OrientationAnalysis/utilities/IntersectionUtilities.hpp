#pragma once

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

#include <array>
namespace nx
{
namespace IntersectionUtilities
{
inline constexpr float k_Epsilon = 1e-8;
inline constexpr uint32_t width = 310;
inline constexpr uint32_t height = 150;

/**
 *@brief Bilinear Interpolator implements function to execute a 2D interpolation
 */
using Vec3f = nx::core::Vec3<float>;

inline float deg2rad(float deg)
{
  return deg * nx::core::Constants::k_PiOver180<float>;
}

inline float clamp(float lo, float hi, float v)
{
  return std::max(lo, std::min(hi, v));
}

inline std::array<float, 6> GetBoundingBoxAtTri(nx::core::TriangleGeom& tri, size_t triId)
{
  nx::core::IGeometry::SharedTriList& triList = tri.getFacesRef();
  // nx::core::IGeometry::SharedVertexList& vertList = tri.getVerticesRef();

  nx::core::uint64* Tri = &triList[triId];
  std::array<nx::core::Point3Df, 3> triCoords;
  tri.getFaceCoordinates(triId, triCoords);
  nx::core::Point3Df vert1 = triCoords[0];
  nx::core::Point3Df vert2 = triCoords[1];
  nx::core::Point3Df vert3 = triCoords[2];
  // nx::core::float32* vert1 = &vertList[Tri[0] * 3];
  // nx::core::float32* vert2 = &vertList[Tri[1]*3+1];
  // nx::core::float32* vert3 = &vertList[Tri[2]*3+2];
  auto xMinMax = std::minmax({vert1[0], vert2[0], vert3[0]});
  auto yMinMax = std::minmax({vert1[1], vert2[1], vert3[1]});
  auto zMinMax = std::minmax({vert1[2], vert2[2], vert3[2]});
  return {xMinMax.first, yMinMax.first, zMinMax.first, xMinMax.second, yMinMax.second, zMinMax.second};
}

/**
 * @brief This version of the rayTriangleIntersect algorithm uses a more traditional algorithm to
 * determine if a point is inside a triangle. This version should be more computationally
 * efficient than the other version
 * @param rayOrigin
 * @param rayDirection
 * @param v0 First Vertex of Triangle
 * @param v1  Second Vertex of Triangle
 * @param v2  Third Vertex of Triangle
 * @param t t part of Barycentric Coord
 * @param u u part of Barycentric Coord
 * @param v v part of Barycentric Coord
 * @return
 */
inline bool RayTriangleIntersect(const Vec3f& rayOrigin, const Vec3f& rayDirection, const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, Vec3f& barycentricCoord)
{
  // compute plane's normal
  Vec3f v0v1 = v1 - v0;
  Vec3f v0v2 = v2 - v0;
  // no need to normalize
  Vec3f N = v0v1.cross(v0v2); // N
  float denom = N.dot(N);

  // Step 1: finding P

  // check if ray and plane are parallel ?
  float NdotRayDirection = N.dot(rayDirection);

  if(fabs(NdotRayDirection) < k_Epsilon) // almost 0
  {
    return false;
  } // they are parallel so they don't intersect !

  // compute d parameter using equation 2
  float d = -N.dot(v0);

  // compute t (equation 3)
  barycentricCoord[2] = -(N.dot(rayOrigin) + d) / NdotRayDirection;

  // check if the triangle is in behind the ray
  if(barycentricCoord[2] < 0)
  {
    return false;
  } // the triangle is behind

  // compute the intersection point using equation 1
  Vec3f P = rayOrigin + (rayDirection * barycentricCoord[2]);

  // Step 2: inside-outside test
  Vec3f C; // vector perpendicular to triangle's plane

  // edge 0
  Vec3f edge0 = v1 - v0;
  Vec3f vp0 = P - v0;
  C = edge0.cross(vp0);
  if(N.dot(C) < 0)
  {
    return false;
  } // P is on the right side

  // edge 1
  Vec3f edge1 = v2 - v1;
  Vec3f vp1 = P - v1;
  C = edge1.cross(vp1);
  if((barycentricCoord[0] = N.dot(C)) < 0)
  {
    return false;
  } // P is on the right side

  // edge 2
  Vec3f edge2 = v0 - v2;
  Vec3f vp2 = P - v2;
  C = edge2.cross(vp2);
  if((barycentricCoord[1] = N.dot(C)) < 0)
  {
    return false; // P is on the right side;
  }

  barycentricCoord[0] /= denom;
  barycentricCoord[1] /= denom;
  barycentricCoord[2] = 1 - barycentricCoord[0] - barycentricCoord[1];

  return true; // this ray hits the triangle
}

/* Adapted from https://github.com/erich666/jgt-code/blob/master/Volume_02/Number_1/Moller1997a/raytri.c */
/* code rewritten to do tests on the sign of the determinant */
/* the division is before the test of the sign of the det    */
/* and one CROSS has been moved out from the if-else if-else
 * @param rayOrigin
 * @param rayDirection
 * @param v0 First Vertex of Triangle
 * @param v1  Second Vertex of Triangle
 * @param v2  Third Vertex of Triangle
 * @param t part of Barycentric Coord
 * @param u part of Barycentric Coord
 * @param v part of Barycentric Coord
 * @return If the point is within the triangle
 */
inline bool RayTriangleIntersect2(const Vec3f& orig, const Vec3f& dir, const Vec3f& vert0, const Vec3f& vert1, const Vec3f& vert2, Vec3f& bcoord)
{
  Vec3f edge1, edge2, tvec, pvec, qvec;
  double det, inv_det;

  /* find vectors for two edges sharing vert0 */
  edge1 = vert1 - vert0;
  edge2 = vert2 - vert0;

  /* begin calculating determinant - also used to calculate U parameter */
  pvec = dir.cross(edge2);

  /* if determinant is near zero, ray lies in plane of triangle */
  det = edge1.dot(pvec);

  /* calculate distance from vert0 to ray origin */
  tvec = orig - vert0;

  inv_det = 1.0 / det;

  qvec = tvec.cross(edge1);

  if(det > k_Epsilon)
  {
    bcoord[0] = tvec.dot(pvec);
    if(bcoord[0] < 0.0 || bcoord[0] > det)
    {
      return false;
    }

    /* calculate V parameter and test bounds */
    // bcoord[1] = DOT(dir, qvec);
    bcoord[1] = dir.dot(qvec);
    if(bcoord[1] < 0.0 || bcoord[0] + bcoord[1] > det)
    {
      return false;
    }
  }
  else if(det < -k_Epsilon)
  {
    /* calculate U parameter and test bounds */
    bcoord[0] = tvec.dot(pvec);
    if(bcoord[0] > 0.0 || bcoord[0] < det)
    {
      return false;
    }

    /* calculate V parameter and test bounds */
    bcoord[1] = dir.dot(qvec);
    if(bcoord[1] > 0.0 || bcoord[0] + bcoord[1] < det)
    {
      return false;
    }
  }
  else
  {
    return false;
  } /* ray is parallel to the plane of the triangle */

  // float t = edge2.dotProduct(qvec) * inv_det;
  float u = bcoord[0] * inv_det;
  float v = bcoord[1] * inv_det;

  bcoord[0] = 1 - u - v;
  bcoord[1] = u;
  bcoord[2] = v;

  return true;
}

} // namespace IntersectionUtilities

} // namespace nx
