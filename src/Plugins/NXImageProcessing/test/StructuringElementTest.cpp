#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"

#include "simplnx/Common/Types.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <set>
#include <tuple>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
using OffsetKey = std::tuple<int32, int32, int32>;
using OffsetSet = std::set<OffsetKey>;

// Collapse the generator output to an order-independent set, and prove no duplicates were emitted.
OffsetSet ToSet(const StructuringElement& se)
{
  OffsetSet result;
  for(const SEOffset& o : se.offsets)
  {
    const bool inserted = result.emplace(o.dx, o.dy, o.dz).second;
    REQUIRE(inserted); // no offset should appear twice
  }
  return result;
}

bool Contains(const OffsetSet& s, int32 dx, int32 dy, int32 dz)
{
  return s.find({dx, dy, dz}) != s.end();
}

// ---------------------------------------------------------------------------
// Independent reference rasterizations, derived directly from the ITK 5.4.4
// itk::FlatStructuringElement factory implementations (Box/Cross/Ball/Annulus),
// evaluated over the [-r, r]^3 bounding box. These are intentionally written
// separately from the implementation so a shared formula bug cannot hide.
// ---------------------------------------------------------------------------

// Box: the full rectangle.
OffsetSet RefBox(int32 rx, int32 ry, int32 rz)
{
  OffsetSet s;
  for(int32 dz = -rz; dz <= rz; ++dz)
    for(int32 dy = -ry; dy <= ry; ++dy)
      for(int32 dx = -rx; dx <= rx; ++dx)
        s.emplace(dx, dy, dz);
  return s;
}

// Cross: only the axes (at most one nonzero component).
OffsetSet RefCross(int32 rx, int32 ry, int32 rz)
{
  OffsetSet s;
  for(int32 dz = -rz; dz <= rz; ++dz)
    for(int32 dy = -ry; dy <= ry; ++dy)
      for(int32 dx = -rx; dx <= rx; ++dx)
      {
        const int32 nonzero = (dx != 0 ? 1 : 0) + (dy != 0 ? 1 : 0) + (dz != 0 ? 1 : 0);
        if(nonzero <= 1)
          s.emplace(dx, dy, dz);
      }
  return s;
}

// Ball (non-parametric): ellipsoid membership. ITK builds an ellipsoid whose
// half-axis along dimension i is 0.5 * axis = 0.5 * (2*r_i + 1) = r_i + 0.5, and
// includes a lattice point when sum_i (d_i / (r_i + 0.5))^2 <= 1.
double EllipsoidValue(int32 dx, int32 dy, int32 dz, double hx, double hy, double hz)
{
  const double fx = static_cast<double>(dx) / hx;
  const double fy = static_cast<double>(dy) / hy;
  const double fz = static_cast<double>(dz) / hz;
  return fx * fx + fy * fy + fz * fz;
}

OffsetSet RefBall(int32 rx, int32 ry, int32 rz)
{
  const double hx = rx + 0.5;
  const double hy = ry + 0.5;
  const double hz = rz + 0.5;
  OffsetSet s;
  for(int32 dz = -rz; dz <= rz; ++dz)
    for(int32 dy = -ry; dy <= ry; ++dy)
      for(int32 dx = -rx; dx <= rx; ++dx)
        if(EllipsoidValue(dx, dy, dz, hx, hy, hz) <= 1.0)
          s.emplace(dx, dy, dz);
  return s;
}

// Annulus (non-parametric, thickness = 1, includeCenter = false): the outer
// Ball with the inner Ball removed. ITK's inner ellipsoid half-axis is
// 0.5 * max(2*r_i + 1 - 2*thickness, 1).
OffsetSet RefAnnulus(int32 rx, int32 ry, int32 rz)
{
  constexpr int32 thickness = 1;
  const double outerHx = rx + 0.5;
  const double outerHy = ry + 0.5;
  const double outerHz = rz + 0.5;
  const double innerHx = 0.5 * std::max(2 * rx + 1 - 2 * thickness, 1);
  const double innerHy = 0.5 * std::max(2 * ry + 1 - 2 * thickness, 1);
  const double innerHz = 0.5 * std::max(2 * rz + 1 - 2 * thickness, 1);
  OffsetSet s;
  for(int32 dz = -rz; dz <= rz; ++dz)
    for(int32 dy = -ry; dy <= ry; ++dy)
      for(int32 dx = -rx; dx <= rx; ++dx)
      {
        const bool outer = EllipsoidValue(dx, dy, dz, outerHx, outerHy, outerHz) <= 1.0;
        const bool inner = EllipsoidValue(dx, dy, dz, innerHx, innerHy, innerHz) <= 1.0;
        if(outer && !inner)
          s.emplace(dx, dy, dz);
      }
  return s;
}
} // namespace

TEST_CASE("ImageProcessing::StructuringElement enum values are parity-critical", "[ImageProcessing][StructuringElement]")
{
  // Must match legacy ChoicesParameter {"Annulus","Ball","Box","Cross"} index,
  // cast directly to itk::simple::KernelEnum {sitkAnnulus=0,sitkBall=1,sitkBox=2,sitkCross=3}.
  STATIC_REQUIRE(static_cast<uint64>(KernelType::Annulus) == 0);
  STATIC_REQUIRE(static_cast<uint64>(KernelType::Ball) == 1);
  STATIC_REQUIRE(static_cast<uint64>(KernelType::Box) == 2);
  STATIC_REQUIRE(static_cast<uint64>(KernelType::Cross) == 3);
}

TEST_CASE("ImageProcessing::StructuringElement Box rasterization", "[ImageProcessing][StructuringElement]")
{
  SECTION("radius {1,1,1}")
  {
    const auto se = MakeStructuringElement(KernelType::Box, {1, 1, 1});
    const auto s = ToSet(se);
    REQUIRE(se.radius == std::array<int32, 3>{1, 1, 1});
    REQUIRE(s.size() == 27); // (2*1+1)^3
    REQUIRE(Contains(s, 0, 0, 0));
    REQUIRE(Contains(s, 1, 1, 1));
    REQUIRE(Contains(s, -1, -1, -1));
    REQUIRE(s == RefBox(1, 1, 1));
  }

  SECTION("asymmetric radius {2,1,0} (2D)")
  {
    const auto se = MakeStructuringElement(KernelType::Box, {2, 1, 0});
    const auto s = ToSet(se);
    REQUIRE(s.size() == 15); // 5 * 3 * 1
    REQUIRE(Contains(s, 2, 1, 0));
    REQUIRE(Contains(s, -2, -1, 0));
    REQUIRE_FALSE(Contains(s, 0, 0, 1)); // rz == 0 => no out-of-plane offsets
    REQUIRE(s == RefBox(2, 1, 0));
  }
}

TEST_CASE("ImageProcessing::StructuringElement Cross rasterization", "[ImageProcessing][StructuringElement]")
{
  SECTION("radius {1,1,1}")
  {
    const auto se = MakeStructuringElement(KernelType::Cross, {1, 1, 1});
    const auto s = ToSet(se);
    REQUIRE(s.size() == 7); // center + 2*(1+1+1)
    REQUIRE(Contains(s, 0, 0, 0));
    REQUIRE(Contains(s, 1, 0, 0));
    REQUIRE(Contains(s, 0, -1, 0));
    REQUIRE(Contains(s, 0, 0, 1));
    REQUIRE_FALSE(Contains(s, 1, 1, 0)); // two nonzero components => not on the cross
    REQUIRE(s == RefCross(1, 1, 1));
  }

  SECTION("asymmetric radius {2,1,0} (2D)")
  {
    const auto se = MakeStructuringElement(KernelType::Cross, {2, 1, 0});
    const auto s = ToSet(se);
    REQUIRE(s.size() == 7); // center + 2*(2+1+0)
    REQUIRE(Contains(s, 2, 0, 0));
    REQUIRE(Contains(s, 0, 1, 0));
    REQUIRE_FALSE(Contains(s, 1, 1, 0));
    REQUIRE_FALSE(Contains(s, 0, 0, 1));
    REQUIRE(s == RefCross(2, 1, 0));
  }
}

TEST_CASE("ImageProcessing::StructuringElement Ball rasterization", "[ImageProcessing][StructuringElement]")
{
  SECTION("radius {1,1,1}")
  {
    const auto se = MakeStructuringElement(KernelType::Ball, {1, 1, 1});
    const auto s = ToSet(se);
    // 3x3x3 minus the 8 corners (corner distance 3*(1/1.5)^2 = 1.333 > 1).
    REQUIRE(s.size() == 19);
    REQUIRE(Contains(s, 0, 0, 0));
    REQUIRE(Contains(s, 1, 0, 0));       // face neighbor
    REQUIRE(Contains(s, 1, 1, 0));       // edge neighbor: 2*(1/1.5)^2 = 0.889 <= 1
    REQUIRE_FALSE(Contains(s, 1, 1, 1)); // corner excluded
    REQUIRE(s == RefBall(1, 1, 1));
  }

  SECTION("asymmetric radius {2,1,0} (2D)")
  {
    const auto se = MakeStructuringElement(KernelType::Ball, {2, 1, 0});
    const auto s = ToSet(se);
    REQUIRE(s.size() == 11);
    REQUIRE(Contains(s, 0, 0, 0));
    REQUIRE(Contains(s, 2, 0, 0)); // (2/2.5)^2 = 0.64 <= 1
    REQUIRE(Contains(s, 1, 1, 0)); // 0.16 + 0.444 = 0.604 <= 1
    REQUIRE(Contains(s, 0, 1, 0));
    REQUIRE_FALSE(Contains(s, 2, 1, 0)); // 0.64 + 0.444 = 1.084 > 1
    REQUIRE(s == RefBall(2, 1, 0));
  }
}

TEST_CASE("ImageProcessing::StructuringElement Annulus rasterization", "[ImageProcessing][StructuringElement]")
{
  SECTION("radius {1,1,1}")
  {
    const auto se = MakeStructuringElement(KernelType::Annulus, {1, 1, 1});
    const auto s = ToSet(se);
    // Ball (19) minus the inner region. For r=1, thickness=1 the inner ellipsoid
    // collapses to just the center, so the annulus is the Ball shell without center.
    REQUIRE(s.size() == 18);
    REQUIRE_FALSE(Contains(s, 0, 0, 0)); // center excluded (includeCenter == false)
    REQUIRE(Contains(s, 1, 0, 0));
    REQUIRE(Contains(s, 1, 1, 0));
    REQUIRE_FALSE(Contains(s, 1, 1, 1)); // still outside the outer Ball
    REQUIRE(s == RefAnnulus(1, 1, 1));
  }

  SECTION("asymmetric radius {2,1,0} (2D)")
  {
    const auto se = MakeStructuringElement(KernelType::Annulus, {2, 1, 0});
    const auto s = ToSet(se);
    REQUIRE(s.size() == 8);
    REQUIRE_FALSE(Contains(s, 0, 0, 0));
    REQUIRE(Contains(s, 2, 0, 0)); // outer, and inner excludes only |dx|<=1 on the dy=0 line
    REQUIRE(Contains(s, -2, 0, 0));
    REQUIRE(Contains(s, 1, 1, 0));
    REQUIRE(Contains(s, 0, 1, 0));
    REQUIRE_FALSE(Contains(s, 1, 0, 0)); // inside the inner ellipsoid
    REQUIRE(s == RefAnnulus(2, 1, 0));
  }
}
