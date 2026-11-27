#include "simplnx/Utilities/IntersectionUtilities.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace nx::core;

TEST_CASE("Simplnx::IntersectionUtilities::Moller-Trumbore Intersection Test (Cached)", "[Simplnx][IntersectionUtilities]")
{
  using PointTType = Eigen::Vector3<float32>;
  intersection_utilities::MTPointsCache<float32> cache;
  cache.pointA = PointTType{0.5, 0, 0};
  cache.pointB = PointTType{-0.5, 0.5, 0};
  cache.pointC = PointTType{-0.5, -0.5, 0};

  cache.edge1 = cache.pointB - cache.pointA;
  cache.edge2 = cache.pointC - cache.pointA;
  {
    // Case 1 (Intersection)
    cache.origin = PointTType{0, 0, -1};
    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    REQUIRE(intersection_utilities::MTIntersection(PointTType{0, 0, 1}, cache).has_value());
  }

  {
    // Case 2 (Intersection)
    cache.origin = PointTType{0, 0, -1};
    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    PointTType dirVec = PointTType{0.1, 0.1, 1};
    dirVec.normalize();

    REQUIRE(intersection_utilities::MTIntersection(dirVec, cache).has_value());
  }

  {
    // Case 3 (No Intersection [Origin Above])
    cache.origin = PointTType{0, 0, 1};
    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    REQUIRE(!intersection_utilities::MTIntersection(PointTType{0, 0, 1}, cache).has_value());
  }

  {
    // Case 4 (No Intersection [Ray Parallel])
    cache.origin = PointTType{0, 0, -1};
    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    REQUIRE(!intersection_utilities::MTIntersection(PointTType{0, 1, 0}, cache).has_value());
  }

  {
    // Case 5 (No Intersection [Misaligned Origin])
    cache.origin = PointTType{10, 0, -1};
    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    REQUIRE(!intersection_utilities::MTIntersection(PointTType{0, 0, 1}, cache).has_value());
  }

  {
    // Case 6 (No Intersection [Wrong Direction])
    cache.origin = PointTType{0, 0, -1};
    cache.sDist = cache.origin - cache.pointA;
    cache.sCrossE1 = cache.sDist.cross(cache.edge1);

    REQUIRE(!intersection_utilities::MTIntersection(PointTType{0, 0, -1}, cache).has_value());
  }
}

TEST_CASE("Simplnx::IntersectionUtilities::Moller-Trumbore Intersection Test", "[Simplnx][IntersectionUtilities]")
{
  using PointTType = Eigen::Vector3<float32>;
  PointTType pointA = PointTType{0.5, 0, 0};
  PointTType pointB = PointTType{-0.5, 0.5, 0};
  PointTType pointC = PointTType{-0.5, -0.5, 0};

  {
    // Case 1 (Intersection)
    REQUIRE(intersection_utilities::MTIntersection(PointTType{0, 0, -1}, PointTType{0, 0, 1}, pointA, pointB, pointC).has_value());
  }

  {
    // Case 2 (Intersection)
    PointTType dirVec = PointTType{0.1, 0.1, 1};
    dirVec.normalize();
    REQUIRE(intersection_utilities::MTIntersection(PointTType{0, 0, -1}, dirVec, pointA, pointB, pointC).has_value());
  }

  {
    // Case 3 (No Intersection [Origin Above])
    REQUIRE(!intersection_utilities::MTIntersection(PointTType{0, 0, 1}, PointTType{0, 0, 1}, pointA, pointB, pointC).has_value());
  }

  {
    // Case 4 (No Intersection [Ray Parallel])
    REQUIRE(!intersection_utilities::MTIntersection(PointTType{0, 0, -1}, PointTType{0, 1, 0}, pointA, pointB, pointC).has_value());
  }

  {
    // Case 5 (No Intersection [Misaligned Origin])
    REQUIRE(!intersection_utilities::MTIntersection(PointTType{10, 0, -1}, PointTType{0, 0, 1}, pointA, pointB, pointC).has_value());
  }

  {
    // Case 6 (No Intersection [Wrong Direction])
    REQUIRE(!intersection_utilities::MTIntersection(PointTType{0, 0, -1}, PointTType{0, 0, -1}, pointA, pointB, pointC).has_value());
  }
}
