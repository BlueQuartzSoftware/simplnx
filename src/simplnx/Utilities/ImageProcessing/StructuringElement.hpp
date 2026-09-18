#pragma once

#include "simplnx/Common/Types.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace nx::core::ImageProcessing
{
/**
 * @brief Shape of a flat structuring element (kernel) used by the grayscale morphology engine.
 *
 * The underlying values are PARITY-CRITICAL. The legacy ITKImageProcessing morphology filters
 * store the kernel choice as a ChoicesParameter index over {"Annulus","Ball","Box","Cross"} and
 * cast that index directly to itk::simple::KernelEnum, which is defined as
 * {sitkAnnulus = 0, sitkBall = 1, sitkBox = 2, sitkCross = 3}. Keeping these values identical means
 * a saved pipeline's kernel index selects the same shape in the ITK-free engine. Do not reorder.
 */
enum class KernelType : uint64
{
  Annulus = 0,
  Ball = 1,
  Box = 2,
  Cross = 3
};

/**
 * @brief A single "on" element of the structuring element expressed as an integer offset
 * (dx, dy, dz) relative to the kernel center.
 */
struct SEOffset
{
  int32 dx;
  int32 dy;
  int32 dz;
};

/**
 * @brief A rasterized flat structuring element: the per-axis radius plus the list of all offsets
 * where the SE mask is "on". Later morphology passes gather/reduce over @ref offsets.
 *
 * @c offsets includes {0,0,0} for Box/Ball/Cross. For Annulus the center is excluded (ITK's
 * includeCenter defaults to false).
 */
struct StructuringElement
{
  std::array<int32, 3> radius{1, 1, 1}; // rx, ry, rz
  std::vector<SEOffset> offsets;        // all (dx,dy,dz) where the SE mask is true
};

namespace detail
{
// Parameters the ITK Annulus factory is invoked with for the default (parameter-less) Annulus
// kernel this engine reproduces. These mirror ITK's own defaults for
// itk::FlatStructuringElement<Dim>::Annulus(radius): thickness = 1, includeCenter = false,
// radiusIsParametric = false. That yields a proper Ball "shell" (outer Ball minus inner Ball).
//
// PARITY NOTE (settled): the legacy SimpleITK-derived CreateKernel (sitkCommon.hpp) calls
// Annulus(radius, false) -- false binds to ITK's `unsigned int thickness` slot => thickness 0,
// collapsing inner==outer into an EMPTY kernel. That is a bug in the legacy wrapper, NOT the
// parity target: this engine intentionally reproduces ITK's real (non-empty) Annulus with
// thickness = 1. Annulus is validated computed-expected, not against the (empty-SE) live legacy.
// Do NOT lower k_AnnulusThickness to 0 to chase the old bug.
constexpr int32 k_AnnulusThickness = 1;

/**
 * @brief Test whether integer offset (dx,dy,dz) lies inside/on an axis-aligned ellipsoid.
 * @param hx,hy,hz Per-axis HALF-axis lengths (0.5 * full axis). Matches ITK's
 * EllipsoidInteriorExteriorSpatialFunction, which divides each component by 0.5 * axis and
 * includes the point when the sum of squares is <= 1.
 */
inline bool InEllipsoid(int32 dx, int32 dy, int32 dz, double hx, double hy, double hz)
{
  const double fx = static_cast<double>(dx) / hx;
  const double fy = static_cast<double>(dy) / hy;
  const double fz = static_cast<double>(dz) / hz;
  return (fx * fx + fy * fy + fz * fz) <= 1.0;
}
} // namespace detail

/**
 * @brief Build the flat structuring element for @p type at the given per-axis @p radius.
 *
 * Reproduces the rasterization of ITK 5.4.4 @c itk::FlatStructuringElement<Dim> factories over the
 * @c [-r, r]^3 bounding box:
 * - Box: the full rectangle.
 * - Cross: only offsets with at most one nonzero component (the axes).
 * - Ball (non-parametric): ellipsoid membership; half-axis i is r_i + 0.5 (= 0.5*(2*r_i+1)), point
 *   included when sum_i (d_i/(r_i+0.5))^2 <= 1.
 * - Annulus (non-parametric): outer Ball minus inner Ball; inner half-axis i is
 *   0.5 * max(2*r_i + 1 - 2*thickness, 1); center excluded.
 *
 * A radius of 0 on an axis degenerates that axis to a single plane (so rz == 0 yields a 2D SE); a
 * negative radius yields an empty range on that axis.
 */
inline StructuringElement MakeStructuringElement(KernelType type, const std::array<int32, 3>& radius)
{
  const int32 rx = radius[0];
  const int32 ry = radius[1];
  const int32 rz = radius[2];

  StructuringElement se;
  se.radius = radius;

  // Ellipsoid half-axes (shared by Ball and the Annulus outer shell).
  const double outerHx = rx + 0.5;
  const double outerHy = ry + 0.5;
  const double outerHz = rz + 0.5;

  // Annulus inner-shell half-axes (only used for Annulus).
  const double innerHx = 0.5 * std::max(2 * rx + 1 - 2 * detail::k_AnnulusThickness, 1);
  const double innerHy = 0.5 * std::max(2 * ry + 1 - 2 * detail::k_AnnulusThickness, 1);
  const double innerHz = 0.5 * std::max(2 * rz + 1 - 2 * detail::k_AnnulusThickness, 1);

  // Reserve an upper bound (the full bounding box) to avoid reallocation churn.
  // Compute in 64-bit to avoid signed-overflow UB on the products for large radii.
  if(rx >= 0 && ry >= 0 && rz >= 0)
  {
    const usize bound = static_cast<usize>(2LL * rx + 1) * static_cast<usize>(2LL * ry + 1) * static_cast<usize>(2LL * rz + 1);
    se.offsets.reserve(bound);
  }

  // Iterate the bounding box with X fastest-moving (matches simplnx's flat layout).
  for(int32 dz = -rz; dz <= rz; ++dz)
  {
    for(int32 dy = -ry; dy <= ry; ++dy)
    {
      for(int32 dx = -rx; dx <= rx; ++dx)
      {
        bool on = false;
        switch(type)
        {
        case KernelType::Box: {
          on = true;
          break;
        }
        case KernelType::Cross: {
          const int32 nonzero = (dx != 0 ? 1 : 0) + (dy != 0 ? 1 : 0) + (dz != 0 ? 1 : 0);
          on = (nonzero <= 1);
          break;
        }
        case KernelType::Ball: {
          on = detail::InEllipsoid(dx, dy, dz, outerHx, outerHy, outerHz);
          break;
        }
        case KernelType::Annulus: {
          // Outer Ball with the inner Ball removed. The center is always inside the inner
          // ellipsoid, so it is excluded here (ITK's includeCenter == false).
          const bool outer = detail::InEllipsoid(dx, dy, dz, outerHx, outerHy, outerHz);
          const bool inner = detail::InEllipsoid(dx, dy, dz, innerHx, innerHy, innerHz);
          on = outer && !inner;
          break;
        }
        }

        if(on)
        {
          se.offsets.push_back(SEOffset{dx, dy, dz});
        }
      }
    }
  }

  return se;
}
} // namespace nx::core::ImageProcessing
