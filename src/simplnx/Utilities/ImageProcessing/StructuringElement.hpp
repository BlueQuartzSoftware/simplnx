#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
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

/** @brief Largest radius whose moving-window offsets can also represent an adjacent offset. */
inline constexpr uint32 k_MaxKernelRadius = static_cast<uint32>(std::numeric_limits<int32>::max() - 1);
inline constexpr int32 k_KernelRadiusLengthError = -88000;
inline constexpr int32 k_KernelRadiusRangeError = -88001;
inline constexpr int32 k_KernelRadiusCountOverflowError = -88002;
inline constexpr int32 k_KernelRadiusCapacityError = -88003;
inline constexpr int32 k_KernelConstructionError = -88004;

/**
 * @brief Checks kernel representation limits without constructing offsets.
 * @param radius X, Y and Z radii in cells. Each radius must be at most k_MaxKernelRadius.
 * @return Error for an invalid axis count, radius, bounding-box count or vector element count.
 * @note The vector element limit also bounds the allocation byte count on supported 64-bit targets.
 * No workload or available-memory limit applies.
 */
inline Result<> ValidateKernelRadius(nonstd::span<const uint32> radius)
{
  if(radius.size() != 3)
  {
    return MakeErrorResult(k_KernelRadiusLengthError, fmt::format("Kernel Radius has {} components. Supply exactly 3 components (X, Y, Z).", radius.size()));
  }
  constexpr std::array<const char*, 3> axes = {"X", "Y", "Z"};
  for(usize axis = 0; axis < 3; ++axis)
  {
    if(radius[axis] > k_MaxKernelRadius)
    {
      return MakeErrorResult(k_KernelRadiusRangeError, fmt::format("Kernel Radius {}={} exceeds the maximum {} cells. Reduce this radius.", axes.at(axis), radius[axis], k_MaxKernelRadius));
    }
  }
  uint64 count = 1;
  for(const uint32 value : radius)
  {
    const uint64 extent = (2 * static_cast<uint64>(value)) + 1;
    if(count > std::numeric_limits<uint64>::max() / extent)
    {
      return MakeErrorResult(k_KernelRadiusCountOverflowError,
                             fmt::format("Kernel Radius (X={}, Y={}, Z={}) overflows the bounding-box count (2*X+1)*(2*Y+1)*(2*Z+1). Reduce the radii.", radius[0], radius[1], radius[2]));
    }
    count *= extent;
  }
  const usize capacity = std::vector<SEOffset>().max_size();
  if(count > capacity)
  {
    return MakeErrorResult(k_KernelRadiusCapacityError,
                           fmt::format("Kernel Radius (X={}, Y={}, Z={}) gives bounding-box element count (2*X+1)*(2*Y+1)*(2*Z+1)={} above the offset vector element count limit {}. Reduce the radii.",
                                       radius[0], radius[1], radius[2], count, capacity));
  }
  return {};
}

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
inline bool InEllipsoid(int64 dx, int64 dy, int64 dz, double hx, double hy, double hz)
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
 * negative radius returns an empty element before arithmetic.
 * @param type Kernel shape.
 * @param radius Signed X, Y and Z radii in cells.
 * @return Rasterized offsets in X-fastest order.
 * @pre The caller selects small fixed radii. User parameters use MakeCheckedStructuringElement instead.
 * @throws std::bad_alloc If offset allocation fails.
 * @throws std::length_error If the offset count exceeds the vector capacity.
 */
inline StructuringElement MakeStructuringElement(KernelType type, const std::array<int32, 3>& radius)
{
  const int64 rx = radius[0];
  const int64 ry = radius[1];
  const int64 rz = radius[2];

  StructuringElement se;
  se.radius = radius;
  if(rx < 0 || ry < 0 || rz < 0)
  {
    return se;
  }

  // Ellipsoid half-axes (shared by Ball and the Annulus outer shell).
  const double outerHx = rx + 0.5;
  const double outerHy = ry + 0.5;
  const double outerHz = rz + 0.5;

  // Annulus inner-shell half-axes (only used for Annulus).
  const double innerHx = 0.5 * std::max<int64>(2 * rx + 1 - 2 * detail::k_AnnulusThickness, 1);
  const double innerHy = 0.5 * std::max<int64>(2 * ry + 1 - 2 * detail::k_AnnulusThickness, 1);
  const double innerHz = 0.5 * std::max<int64>(2 * rz + 1 - 2 * detail::k_AnnulusThickness, 1);

  // A checked bounding-box reservation avoids reallocation without wrapping the element count.
  usize bound = 1;
  bool boundFits = true;
  for(const int32 value : radius)
  {
    const uint64 extent = (2 * static_cast<uint64>(value)) + 1;
    if(extent > std::numeric_limits<usize>::max() || bound > std::numeric_limits<usize>::max() / extent)
    {
      boundFits = false;
      break;
    }
    bound *= static_cast<usize>(extent);
  }
  if(boundFits)
  {
    se.offsets.reserve(bound);
  }

  // Iterate the bounding box with X fastest-moving (matches simplnx's flat layout).
  for(int64 dz = -rz; dz <= rz; ++dz)
  {
    for(int64 dy = -ry; dy <= ry; ++dy)
    {
      for(int64 dx = -rx; dx <= rx; ++dx)
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
          se.offsets.push_back(SEOffset{static_cast<int32>(dx), static_cast<int32>(dy), static_cast<int32>(dz)});
        }
      }
    }
  }

  return se;
}
namespace detail
{
/**
 * @brief Converts construction allocation failures into the shared kernel error.
 * @tparam Build Callable that constructs offsets from the validated shape and radius.
 * @param type Kernel shape passed to build.
 * @param radius User-supplied radii in cells.
 * @param build Construction callable. Tests can inject allocation exceptions here.
 * @return Constructed element, radius error or construction error.
 */
template <class Build>
Result<StructuringElement> MakeCheckedStructuringElementWith(KernelType type, nonstd::span<const uint32> radius, Build&& build)
{
  auto validation = ValidateKernelRadius(radius);
  if(validation.invalid())
  {
    return ConvertInvalidResult<StructuringElement>(std::move(validation));
  }
  try
  {
    return {std::forward<Build>(build)(type, radius)};
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult<StructuringElement>(
        k_KernelConstructionError, fmt::format("Cannot allocate offsets for Kernel Radius (X={}, Y={}, Z={}). Reduce the radii or make more memory available.", radius[0], radius[1], radius[2]));
  } catch(const std::length_error&)
  {
    return MakeErrorResult<StructuringElement>(
        k_KernelConstructionError,
        fmt::format("Cannot construct offsets for Kernel Radius (X={}, Y={}, Z={}): the offset vector length is too large. Reduce the radii.", radius[0], radius[1], radius[2]));
  }
}
} // namespace detail

/**
 * @brief Validates user radii and constructs a structuring element with checked allocation failure handling.
 * @param type Kernel shape.
 * @param radius X, Y and Z radii in cells.
 * @return Constructed element, radius error or construction error.
 */
inline Result<StructuringElement> MakeCheckedStructuringElement(KernelType type, nonstd::span<const uint32> radius)
{
  return detail::MakeCheckedStructuringElementWith(type, radius, [](KernelType shape, nonstd::span<const uint32> values) {
    return MakeStructuringElement(shape, {static_cast<int32>(values[0]), static_cast<int32>(values[1]), static_cast<int32>(values[2])});
  });
}
} // namespace nx::core::ImageProcessing
