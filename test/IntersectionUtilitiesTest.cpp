#include "simplnx/Utilities/IntersectionUtilities.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace nx::core;

TEST_CASE("Simplnx::IntersectionUtilities::Moller-Trumbore Intersection Test (Cached)", "[Simplnx][IntersectionUtilities]")
{
  using PointT = Eigen::Vector3<float32>;
  IntersectionUtilities::MTPointsCache<float32> cache;
  cache.pointA = PointT{0.5, 0, 0};
  cache.pointB = PointT{-0.5, 0.5, 0};
  cache.pointC = PointT{-0.5, -0.5, 0};

  cache.edge1 = cache.pointB - cache.pointA;
  cache.edge2 = cache.pointC - cache.pointA;
  {
    // Case 1 (Intersection)
    cache.origin = PointT{0, 0, -1};
    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    REQUIRE(IntersectionUtilities::MTIntersection(PointT{0, 0, 1}, cache).has_value());
  }

  {
    // Case 2 (Intersection)
    cache.origin = PointT{0, 0, -1};
    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    PointT dirVec = PointT{0.1, 0.1, 1};
    dirVec.normalize();

    REQUIRE(IntersectionUtilities::MTIntersection(dirVec, cache).has_value());
  }

  {
    // Case 3 (No Intersection [Origin Above])
    cache.origin = PointT{0, 0, 1};
    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    REQUIRE(!IntersectionUtilities::MTIntersection(PointT{0, 0, 1}, cache).has_value());
  }

  {
    // Case 4 (No Intersection [Ray Parallel])
    cache.origin = PointT{0, 0, -1};
    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    REQUIRE(!IntersectionUtilities::MTIntersection(PointT{0, 1, 0}, cache).has_value());
  }

  {
    // Case 5 (No Intersection [Misaligned Origin])
    cache.origin = PointT{10, 0, -1};
    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    REQUIRE(!IntersectionUtilities::MTIntersection(PointT{0, 0, 1}, cache).has_value());
  }

  {
    // Case 6 (No Intersection [Wrong Direction])
    cache.origin = PointT{0, 0, -1};
    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    REQUIRE(!IntersectionUtilities::MTIntersection(PointT{0, 0, -1}, cache).has_value());
  }
}

TEST_CASE("Simplnx::IntersectionUtilities::Moller-Trumbore Intersection Test", "[Simplnx][IntersectionUtilities]")
{
  using PointT = Eigen::Vector3<float32>;
  PointT pointA = PointT{0.5, 0, 0};
  PointT pointB = PointT{-0.5, 0.5, 0};
  PointT pointC = PointT{-0.5, -0.5, 0};

  {
    // Case 1 (Intersection)
    REQUIRE(IntersectionUtilities::MTIntersection(PointT{0, 0, -1}, PointT{0, 0, 1}, pointA, pointB, pointC).has_value());
  }

  {
    // Case 2 (Intersection)
    PointT dirVec = PointT{0.1, 0.1, 1};
    dirVec.normalize();
    REQUIRE(IntersectionUtilities::MTIntersection(PointT{0, 0, -1}, dirVec, pointA, pointB, pointC).has_value());
  }

  {
    // Case 3 (No Intersection [Origin Above])
    REQUIRE(!IntersectionUtilities::MTIntersection(PointT{0, 0, 1}, PointT{0, 0, 1}, pointA, pointB, pointC).has_value());
  }

  {
    // Case 4 (No Intersection [Ray Parallel])
    REQUIRE(!IntersectionUtilities::MTIntersection(PointT{0, 0, -1}, PointT{0, 1, 0}, pointA, pointB, pointC).has_value());
  }

  {
    // Case 5 (No Intersection [Misaligned Origin])
    REQUIRE(!IntersectionUtilities::MTIntersection(PointT{10, 0, -1}, PointT{0, 0, 1}, pointA, pointB, pointC).has_value());
  }

  {
    // Case 6 (No Intersection [Wrong Direction])
    REQUIRE(!IntersectionUtilities::MTIntersection(PointT{0, 0, -1}, PointT{0, 0, -1}, pointA, pointB, pointC).has_value());
  }
}
