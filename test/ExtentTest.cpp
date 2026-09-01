#include <catch2/catch.hpp>

#include "simplnx/Common/Extent.hpp"

using namespace nx::core;

TEST_CASE("Extent::Construction", "[Extent]")
{
  SECTION("1D extent")
  {
    Extent e({0}, {9});
    REQUIRE(e.dimensions() == 1);
    REQUIRE(e.size(0) == 10);
    REQUIRE(e.totalElements() == 10);
  }

  SECTION("2D extent")
  {
    Extent e({0, 0}, {4, 7});
    REQUIRE(e.dimensions() == 2);
    REQUIRE(e.size(0) == 5);
    REQUIRE(e.size(1) == 8);
    REQUIRE(e.totalElements() == 40);
  }

  SECTION("3D extent")
  {
    Extent e({1, 2, 3}, {3, 5, 6});
    REQUIRE(e.dimensions() == 3);
    REQUIRE(e.size(0) == 3);
    REQUIRE(e.size(1) == 4);
    REQUIRE(e.size(2) == 4);
    REQUIRE(e.totalElements() == 48);
  }

  SECTION("Default construction yields 0 dimensions")
  {
    Extent e;
    REQUIRE(e.dimensions() == 0);
    REQUIRE(e.totalElements() == 0);
  }
}

TEST_CASE("Extent::Contains", "[Extent]")
{
  SECTION("Inner within outer")
  {
    Extent outer({0, 0}, {9, 9});
    Extent inner({2, 3}, {5, 7});
    REQUIRE(outer.contains(inner));
    REQUIRE_FALSE(inner.contains(outer));
  }

  SECTION("Partial overlap does not count as contains")
  {
    Extent a({0, 0}, {5, 5});
    Extent b({3, 3}, {8, 8});
    REQUIRE_FALSE(a.contains(b));
    REQUIRE_FALSE(b.contains(a));
  }

  SECTION("Extent contains itself")
  {
    Extent e({1, 2}, {4, 6});
    REQUIRE(e.contains(e));
  }
}

TEST_CASE("Extent::Overlaps", "[Extent]")
{
  SECTION("Overlapping extents")
  {
    Extent a({0, 0}, {5, 5});
    Extent b({3, 3}, {8, 8});
    REQUIRE(a.overlaps(b));
    REQUIRE(b.overlaps(a));
  }

  SECTION("Non-overlapping extents")
  {
    Extent a({0, 0}, {3, 3});
    Extent b({5, 5}, {8, 8});
    REQUIRE_FALSE(a.overlaps(b));
    REQUIRE_FALSE(b.overlaps(a));
  }

  SECTION("Adjacent extents do not overlap")
  {
    // a ends at 3, b starts at 4 — they touch but do not share any element
    Extent a({0, 0}, {3, 3});
    Extent b({4, 0}, {7, 3});
    REQUIRE_FALSE(a.overlaps(b));
    REQUIRE_FALSE(b.overlaps(a));
  }

  SECTION("Sharing a single boundary element overlaps")
  {
    // Both include index 3 in dimension 0
    Extent a({0, 0}, {3, 3});
    Extent b({3, 0}, {7, 3});
    REQUIRE(a.overlaps(b));
    REQUIRE(b.overlaps(a));
  }
}

TEST_CASE("Extent::Intersect", "[Extent]")
{
  SECTION("Overlapping intersection")
  {
    Extent a({0, 0}, {5, 5});
    Extent b({3, 2}, {8, 7});
    Extent result = a.intersect(b);
    REQUIRE(result.dimensions() == 2);
    REQUIRE(result.min[0] == 3);
    REQUIRE(result.min[1] == 2);
    REQUIRE(result.max[0] == 5);
    REQUIRE(result.max[1] == 5);
  }

  SECTION("Non-overlapping intersection yields empty extent")
  {
    Extent a({0, 0}, {3, 3});
    Extent b({5, 5}, {8, 8});
    Extent result = a.intersect(b);
    REQUIRE(result.dimensions() == 0);
  }
}

TEST_CASE("Extent::Equality", "[Extent]")
{
  SECTION("Same extents are equal")
  {
    Extent a({1, 2, 3}, {4, 5, 6});
    Extent b({1, 2, 3}, {4, 5, 6});
    REQUIRE(a == b);
  }

  SECTION("Different extents are not equal")
  {
    Extent a({0, 0}, {5, 5});
    Extent b({0, 0}, {5, 6});
    REQUIRE_FALSE(a == b);
  }

  SECTION("Different dimensions are not equal")
  {
    Extent a({0, 0}, {5, 5});
    Extent b({0, 0, 0}, {5, 5, 5});
    REQUIRE_FALSE(a == b);
  }

  SECTION("Different strides are not equal")
  {
    Extent a({0, 0}, {9, 9}, {1, 1});
    Extent b({0, 0}, {9, 9}, {2, 2});
    REQUIRE_FALSE(a == b);
  }

  SECTION("Same extents with same stride are equal")
  {
    Extent a({0, 0}, {9, 9}, {2, 3});
    Extent b({0, 0}, {9, 9}, {2, 3});
    REQUIRE(a == b);
  }
}

// =============================================================================
// Extent::Stride
// =============================================================================
TEST_CASE("Extent::Stride default (all ones)", "[Extent]")
{
  SECTION("1D default stride")
  {
    Extent e({0}, {9});
    REQUIRE(e.stride.size() == 1);
    REQUIRE(e.stride[0] == 1);
    REQUIRE(e.size(0) == 10);
    REQUIRE(e.totalElements() == 10);
  }

  SECTION("2D default stride")
  {
    Extent e({0, 0}, {4, 7});
    REQUIRE(e.stride.size() == 2);
    REQUIRE(e.stride[0] == 1);
    REQUIRE(e.stride[1] == 1);
    REQUIRE(e.size(0) == 5);
    REQUIRE(e.size(1) == 8);
    REQUIRE(e.totalElements() == 40);
  }

  SECTION("3D default stride")
  {
    Extent e({1, 2, 3}, {3, 5, 6});
    REQUIRE(e.stride.size() == 3);
    for(std::size_t d = 0; d < 3; ++d)
    {
      REQUIRE(e.stride[d] == 1);
    }
    REQUIRE(e.totalElements() == 48);
  }
}

TEST_CASE("Extent::Stride explicit stride", "[Extent]")
{
  SECTION("1D stride 2 evenly divisible")
  {
    // min=0, max=9, stride=2 -> indices 0,2,4,6,8 -> 5 elements
    Extent e({0}, {9}, {2});
    REQUIRE(e.stride[0] == 2);
    REQUIRE(e.size(0) == 5);
    REQUIRE(e.totalElements() == 5);
  }

  SECTION("1D stride 3 evenly divisible")
  {
    // min=0, max=8, stride=3 -> indices 0,3,6 -> 3 elements
    Extent e({0}, {8}, {3});
    REQUIRE(e.size(0) == 3);
  }

  SECTION("1D stride non-divisible (ceiling)")
  {
    // min=0, max=9, stride=3 -> (9-0+1+3-1)/3 = 12/3 = 4 -> indices 0,3,6,9
    Extent e({0}, {9}, {3});
    REQUIRE(e.size(0) == 4);
    REQUIRE(e.totalElements() == 4);
  }

  SECTION("1D stride non-divisible offset start")
  {
    // min=1, max=10, stride=3 -> (10-1+1+3-1)/3 = 12/3 = 4 -> indices 1,4,7,10
    Extent e({1}, {10}, {3});
    REQUIRE(e.size(0) == 4);
  }

  SECTION("1D stride equals range size (single element)")
  {
    // min=0, max=9, stride=10 -> (10+10-1)/10 = 19/10 = 1
    Extent e({0}, {9}, {10});
    REQUIRE(e.size(0) == 1);
  }

  SECTION("2D stride")
  {
    // dim0: min=0, max=9, stride=2 -> 5 elements
    // dim1: min=0, max=11, stride=3 -> 4 elements
    Extent e({0, 0}, {9, 11}, {2, 3});
    REQUIRE(e.size(0) == 5);
    REQUIRE(e.size(1) == 4);
    REQUIRE(e.totalElements() == 20);
  }

  SECTION("3D stride non-divisible")
  {
    // dim0: (4+2-1)/2 = 5/2 = 2
    // dim1: (6+3-1)/3 = 8/3 = 2
    // dim2: (10+4-1)/4 = 13/4 = 3
    Extent e({0, 0, 0}, {3, 5, 9}, {2, 3, 4});
    REQUIRE(e.size(0) == 2);
    REQUIRE(e.size(1) == 2);
    REQUIRE(e.size(2) == 3);
    REQUIRE(e.totalElements() == 12);
  }
}

TEST_CASE("Extent::Stride contains and overlaps are stride-agnostic", "[Extent]")
{
  SECTION("contains ignores stride")
  {
    // contains() operates on min/max bounds only
    Extent outer({0, 0}, {9, 9}, {1, 1});
    Extent inner({2, 3}, {5, 7}, {2, 2});
    REQUIRE(outer.contains(inner));
    REQUIRE_FALSE(inner.contains(outer));
  }

  SECTION("overlaps ignores stride")
  {
    Extent a({0, 0}, {5, 5}, {2, 1});
    Extent b({3, 3}, {8, 8}, {3, 1});
    REQUIRE(a.overlaps(b));
    REQUIRE(b.overlaps(a));
  }
}

TEST_CASE("Extent::Stride intersect uses element-wise max stride", "[Extent]")
{
  SECTION("Intersect takes max stride per dimension")
  {
    Extent a({0, 0}, {9, 9}, {2, 1});
    Extent b({3, 3}, {12, 12}, {1, 3});
    Extent result = a.intersect(b);
    REQUIRE(result.dimensions() == 2);
    REQUIRE(result.min[0] == 3);
    REQUIRE(result.min[1] == 3);
    REQUIRE(result.max[0] == 9);
    REQUIRE(result.max[1] == 9);
    REQUIRE(result.stride[0] == 2); // max(2,1)
    REQUIRE(result.stride[1] == 3); // max(1,3)
  }

  SECTION("Non-overlapping returns empty (no stride)")
  {
    Extent a({0, 0}, {3, 3}, {2, 2});
    Extent b({5, 5}, {8, 8}, {3, 3});
    Extent result = a.intersect(b);
    REQUIRE(result.dimensions() == 0);
  }
}

TEST_CASE("Extent::Stride constructor validation", "[Extent]")
{
  SECTION("Zero stride throws")
  {
    REQUIRE_THROWS_AS(nx::core::Extent({0, 0}, {9, 9}, {2, 0}), std::invalid_argument);
  }
  SECTION("Mismatched stride size throws")
  {
    REQUIRE_THROWS_AS(nx::core::Extent({0, 0}, {9, 9}, {2}), std::invalid_argument);
  }
}
